#include "game/world/gen/pipeline.hpp"

#include "core/log.hpp"

#include <algorithm>
#include <format>
#include <limits>
#include <stdexcept>
#include <tuple>

namespace {

using Clock = std::chrono::steady_clock;

constexpr i32  _unloadSlack = 2;
constexpr i32  _far         = std::numeric_limits<i32>::max() / 2;
constexpr size _maxStages   = std::numeric_limits<u8>::max() - 1;

}

ChunkPipeline::ChunkPipeline(Info info)
: m_stages(std::make_shared<const StageList>(std::move(info.stages))),
  m_seed(info.seed),
  m_complete(static_cast<Status>(m_stages->size())) {
  const StageList& stages = *m_stages;

  if (stages.empty() || stages.size() > _maxStages)
    throw std::logic_error(std::format("ChunkPipeline needs between 1 and {} stages, got {}", _maxStages, stages.size()));

  for (const auto& stage : stages) {
    const Stage::Requirements requirements = stage->requirements();
    if (requirements.radius < 0 || (requirements.radius == 0) != (requirements.neighbors == Access::None))
      throw std::logic_error(std::format("Stage '{}' must declare neighbor access exactly when its radius is above 0", stage->name()));
  }

  if (stages.back()->requirements().neighbors == Access::Write)
    throw std::logic_error(std::format("The last stage ('{}') cannot write into neighbors", stages.back()->name()));

  m_dependency.resize(stages.size());
  for (size i = 0; i < stages.size(); ++i) {
    m_dependency[i] = stages[i]->requirements().radius;
    if (i == 0) continue;

    const Stage::Requirements previous = stages[i - 1]->requirements();
    if (previous.neighbors == Access::Write) m_dependency[i] = std::max(m_dependency[i], previous.radius);
  }

  m_reach.assign(stages.size() + 1, 0);
  for (size i = stages.size(); i-- > 0;)
    m_reach[i] = m_reach[i + 1] + m_dependency[i];

  m_stats.resize(stages.size());

  for (size i = 0; i < stages.size(); ++i)
    logging::game.debug("Stage {} '{}' (dependency radius {})", i + 1, stages[i]->name(), m_dependency[i]);
}

ChunkPipeline::~ChunkPipeline() {
  report();
}

void ChunkPipeline::update(std::span<const ChunkLoader> loaders) {
  collect();

  if (!std::ranges::equal(loaders, m_loaders)) {
    m_loaders.assign(loaders.begin(), loaders.end());
    retarget();
  }

  unload();
  schedule();
}

std::shared_ptr<const Chunk> ChunkPipeline::chunk(ChunkPos pos) const {
  const auto itr = m_entries.find(pos);
  if (itr == m_entries.end() || itr->second.status < m_complete) return nullptr;

  const auto& proto = itr->second.proto;
  return std::shared_ptr<const Chunk>(proto, &proto->chunk);
}

void ChunkPipeline::retarget() {
  for (auto& [pos, entry] : m_entries)
    measure(pos, entry);

  for (const ChunkLoader& loader : m_loaders) {
    const i32 extent = loader.radius + m_reach[0];
    for (i32 z = -extent; z <= extent; ++z) {
      for (i32 x = -extent; x <= extent; ++x) {
        auto [itr, inserted] = m_entries.try_emplace(loader.center + ivec2(x, z));
        if (inserted) measure(itr->first, itr->second);
      }
    }
  }

  std::erase_if(m_jobs, [this](Job& job) {
    const Entry& entry = m_entries.at(job.pos);
    if (entry.target > job.stage) {
      job.task.updatePriority(entry.distance);
      return false;
    }

    if (!job.task.cancel()) return false;
    release(job);
    return true;
  });
}

void ChunkPipeline::collect() {
  std::erase_if(m_jobs, [this](Job& job) {
    const std::optional<Result> result = job.task.take();
    if (!result) return false;

    release(job);
    m_entries.at(job.pos).status = job.stage + 1;

    Stats& stats = m_stats[job.stage];
    ++stats.runs;
    stats.time += result->elapsed;

    logging::game.trace("Chunk ({}, {}) finished '{}'", job.pos.x, job.pos.y, (*m_stages)[job.stage]->name());
    return true;
  });
}

void ChunkPipeline::unload() {
  const i32 limit = m_reach[0] + _unloadSlack;

  std::erase_if(m_entries, [limit](const auto& item) {
    const Entry& entry = item.second;
    return entry.target == 0 && entry.overshoot > limit && entry.readers == 0 && !entry.writer;
  });
}

void ChunkPipeline::schedule() {
  struct Candidate {
    i32      distance;
    Status   status;
    ChunkPos pos;
    Entry*   entry;
  };

  std::vector<Candidate> candidates;
  for (auto& [pos, entry] : m_entries) {
    if (entry.status >= entry.target || entry.writer || entry.readers > 0) continue;
    candidates.push_back(Candidate{.distance = entry.distance, .status = entry.status, .pos = pos, .entry = &entry});
  }

  std::ranges::sort(candidates, [](const Candidate& a, const Candidate& b) {
    return std::tie(a.distance, a.status) < std::tie(b.distance, b.status);
  });

  for (const Candidate& candidate : candidates) {
    const Stage& stage = *(*m_stages)[candidate.entry->status];
    if (!dependenciesMet(candidate.pos, *candidate.entry)) continue;
    if (!leasable(candidate.pos, stage.requirements())) continue;
    submit(candidate.pos, *candidate.entry);
  }
}

void ChunkPipeline::measure(ChunkPos pos, Entry& entry) const {
  entry.distance  = _far;
  entry.overshoot = _far;

  for (const ChunkLoader& loader : m_loaders) {
    const i32 distance = Chunk::distance(pos, loader.center);
    entry.distance     = std::min(entry.distance, distance);
    entry.overshoot    = std::min(entry.overshoot, distance - loader.radius);
  }

  entry.target = targetAt(entry.overshoot);
}

ChunkPipeline::Status ChunkPipeline::targetAt(i32 overshoot) const {
  if (overshoot <= 0) return m_complete;

  for (Status status = m_complete - 1; status > 0; --status)
    if (overshoot <= m_reach[status]) return status;

  return 0;
}

bool ChunkPipeline::dependenciesMet(ChunkPos pos, const Entry& entry) const {
  const i32 radius = m_dependency[entry.status];

  for (i32 z = -radius; z <= radius; ++z) {
    for (i32 x = -radius; x <= radius; ++x) {
      if (x == 0 && z == 0) continue;

      const auto itr = m_entries.find(pos + ivec2(x, z));
      if (itr == m_entries.end() || itr->second.status < entry.status) return false;
    }
  }

  return true;
}

bool ChunkPipeline::leasable(ChunkPos pos, const Stage::Requirements& requirements) const {
  const Entry& self = m_entries.at(pos);
  if (self.writer || self.readers > 0) return false;

  for (i32 z = -requirements.radius; z <= requirements.radius; ++z) {
    for (i32 x = -requirements.radius; x <= requirements.radius; ++x) {
      if (x == 0 && z == 0) continue;

      const auto itr = m_entries.find(pos + ivec2(x, z));
      if (itr == m_entries.end() || itr->second.writer) return false;
      if (requirements.neighbors == Access::Write && itr->second.readers > 0) return false;
    }
  }

  return true;
}

void ChunkPipeline::submit(ChunkPos pos, Entry& entry) {
  const Status              stage        = entry.status;
  const Stage::Requirements requirements = (*m_stages)[stage]->requirements();
  const i32                 radius       = requirements.radius;

  std::vector<std::shared_ptr<ProtoChunk>> chunks;
  std::vector<ChunkPos>                    neighbors;
  chunks.reserve(static_cast<size>((2 * radius + 1) * (2 * radius + 1)));

  for (i32 z = -radius; z <= radius; ++z) {
    for (i32 x = -radius; x <= radius; ++x) {
      const ivec2 offset   = ivec2(x, z);
      const bool  isSelf   = offset == ivec2(0);
      Entry&      neighbor = isSelf ? entry : m_entries.at(pos + offset);

      if (neighbor.proto == nullptr) neighbor.proto = std::make_shared<ProtoChunk>();
      chunks.push_back(neighbor.proto);

      if (isSelf) continue;

      neighbors.push_back(pos + offset);
      if (requirements.neighbors == Access::Write)
        neighbor.writer = true;
      else
        ++neighbor.readers;
    }
  }

  entry.writer = true;

  Task<Result> task([stages = m_stages, stage, pos, requirements, seed = m_seed, chunks = std::move(chunks)] {
    const auto start = Clock::now();

    Region region(Region::Info{
      .pos    = pos,
      .radius = requirements.radius,
      .access = requirements.neighbors,
      .seed   = seed,
      .chunks = chunks
    });
    (*stages)[stage]->generate(region);

    return Result{.elapsed = Clock::now() - start};
  },
    entry.distance);

  m_jobs.push_back(Job{
    .pos       = pos,
    .stage     = stage,
    .access    = requirements.neighbors,
    .neighbors = std::move(neighbors),
    .task      = std::move(task)
  });
}

void ChunkPipeline::release(const Job& job) {
  m_entries.at(job.pos).writer = false;

  for (const ChunkPos pos : job.neighbors) {
    Entry& neighbor = m_entries.at(pos);
    if (job.access == Access::Write)
      neighbor.writer = false;
    else
      --neighbor.readers;
  }
}

void ChunkPipeline::report() const {
  for (size i = 0; i < m_stats.size(); ++i) {
    const Stats& stats = m_stats[i];
    if (stats.runs == 0) continue;

    const f64 average = std::chrono::duration<f64, std::milli>(stats.time).count() / static_cast<f64>(stats.runs);
    logging::game.debug("Stage '{}': {} runs, {:.3f} ms average", (*m_stages)[i]->name(), stats.runs, average);
  }
}

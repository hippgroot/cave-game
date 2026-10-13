#pragma once

#include "client/ui/context.hpp"
#include "client/ui/theme.hpp"

#include <memory>
#include <vector>

class Screen {
  friend class ScreenStack;

  f32 m_time = 0.0f;

public:
  virtual ~Screen() = default;

  Screen(const Screen&)            = delete;
  Screen(Screen&&)                 = delete;
  Screen& operator=(const Screen&) = delete;
  Screen& operator=(Screen&&)      = delete;

  virtual void onEnter() {}
  virtual void onExit() {}
  virtual void onCovered() {}
  virtual void onUncovered() {}

  virtual void draw(const ui::Context& ctx) = 0;

  virtual bool isOpaque() const { return true; }
  virtual bool pausesGame() const { return true; }
  virtual bool closesOnEscape() const { return true; }

  f32 time() const { return m_time; }

protected:
  Screen() = default;
};

class ScreenStack {
  enum class OpType : u8 {
    Push,
    Pop,
    Replace,
    Clear
  };

  struct Op {
    OpType                  type;
    std::unique_ptr<Screen> screen;
  };

  ui::Theme                            m_theme;
  std::vector<std::unique_ptr<Screen>> m_screens;
  std::vector<Op>                      m_pending;

public:
  explicit ScreenStack(ui::Theme theme);
  ~ScreenStack();

  ScreenStack(const ScreenStack&)            = delete;
  ScreenStack(ScreenStack&&)                 = delete;
  ScreenStack& operator=(const ScreenStack&) = delete;
  ScreenStack& operator=(ScreenStack&&)      = delete;

  void push(std::unique_ptr<Screen> screen);
  void pop();
  void replace(std::unique_ptr<Screen> screen);
  void clear();

  template <typename T, typename... Args>
  void push(Args&&... args) {
    push(std::make_unique<T>(std::forward<Args>(args)...));
  }

  template <typename T, typename... Args>
  void replace(Args&&... args) {
    replace(std::make_unique<T>(std::forward<Args>(args)...));
  }

  void draw(f32 dt);

  bool    empty() const;
  Screen* top() const;
  bool    pausesGame() const;

  const ui::Theme& theme() const;

private:
  void flush();
  void enter(std::unique_ptr<Screen> screen);
  void install(std::unique_ptr<Screen> screen);
  void exitTop();
  void exitAll();
};

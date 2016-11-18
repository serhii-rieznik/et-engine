/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/singleton.h>
#include <et/core/containers.h>
#include <et/app/events.h>

namespace et
{

enum class InputAction : uint32_t
{
	KeyDown,
	KeyUp,
	Characters,
	PointerPressed,
	PointerMoved,
	PointerReleased,
	PointerCancelled,
	PointerScrolled
};

enum PointerTypeMask : uint32_t
{
	General = 0x01,
	RightButton = 0x02,
	MiddleButton = 0x04,
};

enum class PointerOrigin : uint32_t
{
	Mouse,
	Trackpad,
	Touchscreen,
	Any
};

using PointerType = uint32_t;

struct PointerInputInfo
{
	uint32_t id = 0;
	float timestamp = 0.0f;
	vec2 pos = vec2(0.0f);
	vec2 normalizedPos = vec2(0.0f);
	vec2 scroll = vec2(0.0f);
	PointerType type = 0;
	PointerOrigin origin = PointerOrigin::Any;
	char tag = 0;

	PointerInputInfo() = default;

	PointerInputInfo(const PointerInputInfo& p) :
		id(p.id), timestamp(p.timestamp), pos(p.pos), normalizedPos(p.normalizedPos), scroll(p.scroll),
		type(p.type), origin(p.origin), tag(p.tag) { }

	PointerInputInfo(PointerInputInfo&& r) : id(r.id), timestamp(r.timestamp), pos(r.pos),
		normalizedPos(r.normalizedPos), scroll(r.scroll), type(r.type), origin(r.origin), tag(r.tag) { }

	PointerInputInfo(PointerType t, const vec2& p, const vec2& np, const vec2& aScroll,
		uint32_t aId, float time, PointerOrigin o) : id(aId), timestamp(time), pos(p), normalizedPos(np),
		scroll(aScroll), type(t), origin(o), tag(0) { }

	PointerInputInfo& operator = (const PointerInputInfo& p)
	{
		id = p.id;
		timestamp = p.timestamp;
		pos = p.pos;
		normalizedPos = p.normalizedPos;
		scroll = p.scroll;
		type = p.type;
		origin = p.origin;
		tag = p.tag;
		return *this;
	}
};

enum GestureTypeMask
{
	GestureTypeMask_Zoom = 0x01,
	GestureTypeMask_Rotate = 0x02,
	GestureTypeMask_Swipe = 0x04,
	GestureTypeMask_Shake = 0x08
};

struct GestureInputInfo
{
	vec2 swipe = vec2(0.0f);
	float zoom = 0.0f;
	float rotation = 0.0f;
	uint32_t mask = 0;

	GestureInputInfo() = default;

	GestureInputInfo(uint32_t m) :
		mask(m) { }

	GestureInputInfo(uint32_t m, float v) : mask(m)
	{
		if (m & GestureTypeMask_Zoom)
			zoom = v;
		else if (m & GestureTypeMask_Rotate)
			rotation = v;
	}

	GestureInputInfo(uint32_t m, float x, float y) :
		swipe(x, y), mask(m) { }
};

class Input : public Singleton<Input>
{
public:
	static bool canGetCurrentPointerInfo();
	static PointerInputInfo currentPointer();

public:
	class KeyboardInputSource;
	class PointerInputSource;
	class GestureInputSource;

	bool isKeyPressed(uint32_t key) const
	{
		return _pressedKeys.count(key) > 0;
	}

	bool isPointerPressed(PointerType type) const;

	void activateSoftwareKeyboard();
	void deactivateSoftwareKeyboard();

public:
	ET_DECLARE_EVENT1(keyPressed, uint32_t)
		ET_DECLARE_EVENT1(charactersEntered, std::string)
		ET_DECLARE_EVENT1(keyReleased, uint32_t)

		ET_DECLARE_EVENT1(pointerPressed, PointerInputInfo)
		ET_DECLARE_EVENT1(pointerMoved, PointerInputInfo)
		ET_DECLARE_EVENT1(pointerReleased, PointerInputInfo)
		ET_DECLARE_EVENT1(pointerCancelled, PointerInputInfo)
		ET_DECLARE_EVENT1(pointerScrolled, PointerInputInfo)

		ET_DECLARE_EVENT1(gesturePerformed, GestureInputInfo)

private:
	Input();
	ET_SINGLETON_COPY_DENY(Input);

	friend class KeyboardInputSource;
	friend class PointerInputSource;
	friend class GestureInputSource;

	void pushKeyboardInputAction(uint32_t key, InputAction action);
	void pushKeyboardInputAction(const std::string&, InputAction action);

	void pushPointerInputAction(const PointerInputInfo& info, InputAction action);
	void pushGestureInputAction(const GestureInputInfo&);

	void addPointerInfo(const PointerInputInfo& info);
	void updatePointerInfo(const PointerInputInfo& info);
	void removePointerInfo(const PointerInputInfo& info);

private:
	using KeysMap = std::map<uint32_t, uint32_t>;
	using PointerInputInfoList = Vector<PointerInputInfo>;

	KeysMap _pressedKeys;
	PointerInputInfoList _pointers;
};

class Input::KeyboardInputSource
{
public:
	void keyPressed(uint32_t key)
	{
		Input::instance().pushKeyboardInputAction(key, InputAction::KeyDown);
	}

	void charactersEntered(const std::string& chars)
	{
		Input::instance().pushKeyboardInputAction(chars, InputAction::Characters);
	}

	void keyReleased(uint32_t key)
	{
		Input::instance().pushKeyboardInputAction(key, InputAction::KeyUp);
	}
};

class Input::PointerInputSource
{
public:
	void pointerPressed(const PointerInputInfo& info)
	{
		Input::instance().pushPointerInputAction(info, InputAction::PointerPressed);
	}

	void pointerMoved(const PointerInputInfo& info)
	{
		Input::instance().pushPointerInputAction(info, InputAction::PointerMoved);
	}

	void pointerReleased(const PointerInputInfo& info)
	{
		Input::instance().pushPointerInputAction(info, InputAction::PointerReleased);
	}

	void pointerCancelled(const PointerInputInfo& info)
	{
		Input::instance().pushPointerInputAction(info, InputAction::PointerCancelled);
	}

	void pointerScrolled(const PointerInputInfo& info)
	{
		Input::instance().pushPointerInputAction(info, InputAction::PointerScrolled);
	}
};

class Input::GestureInputSource
{
public:
	void gesturePerformed(const GestureInputInfo& info)
	{
		Input::instance().pushGestureInputAction(info);
	}
};

class InputHandler : public EventReceiver
{
protected:
	InputHandler(bool connect = true);
	virtual ~InputHandler();

	void connectInputEvents();

protected:
	virtual void onPointerPressed(et::PointerInputInfo) {}
	virtual void onPointerMoved(et::PointerInputInfo) {}
	virtual void onPointerReleased(et::PointerInputInfo) {}
	virtual void onPointerCancelled(et::PointerInputInfo) {}
	virtual void onPointerScrolled(et::PointerInputInfo) {}

	virtual void onKeyPressed(uint32_t) {}
	virtual void onCharactersEntered(std::string) {}
	virtual void onKeyReleased(uint32_t) {}

	virtual void onGesturePerformed(et::GestureInputInfo) {}
};

Input& input();
}

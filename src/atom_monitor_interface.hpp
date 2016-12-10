// atom_monitor_interface.hpp
// Copyright 2016 Steve Palmer

#ifndef ATOM_MONITOR_INTERFACE_HPP_
#define ATOM_MONITOR_INTERFACE_HPP_

#include <set>

#include "common.hpp"
#include "part.hpp"

namespace Atom
{

	class MonitorInterface
		: protected NonCopyable
	{
	public:
		enum class VDGMode {
			UNKNOWN,
			MODE0,  /* 32x16 text & 64x48 block graphics in 0.5KB */
			MODE1A, /* 64x64 4 colour graphics           in 1KB */
			MODE1,  /* 128x64 black & white graphics     in 1KB */
			MODE2A, /* 128x64 4 colour graphics          in 2KB */
			MODE2,  /* 128x96 black & white graphics     in 1.5KB */
			MODE3A, /* 128x96 4 colour graphics          in 3KB */
			MODE3,  /* 128x192 black & white graphics    in 3KB */
			MODE4A, /* 128x192 4 colour graphics         in 6KB */
			MODE4,  /* 256x192 black & white graphics    in 6KB */
		};

		friend std::ostream &operator<<(std::ostream &p_s, const VDGMode &p_mode)
			{
			switch (p_mode)
			{
			case VDGMode::UNKNOWN:
				p_s << "unknown";
				break;
			case VDGMode::MODE0:
				p_s << "mode 0";
				break;
			case VDGMode::MODE1A:
				p_s << "mode 0";
				break;
			case VDGMode::MODE1:
				p_s << "mode 0";
				break;
			case VDGMode::MODE2A:
				p_s << "mode 0";
				break;
			case VDGMode::MODE2:
				p_s << "mode 0";
				break;
			case VDGMode::MODE3A:
				p_s << "mode 0";
				break;
			case VDGMode::MODE3:
				p_s << "mode 0";
				break;
			case VDGMode::MODE4A:
				p_s << "mode 0";
				break;
			case VDGMode::MODE4:
				p_s << "mode 0";
				break;
			default:
				p_s << "error";
				break;
			}
			return p_s;
			}

		class Observer
			: protected NonCopyable
		{
		protected:
			Observer() = default;
		public:
			virtual ~Observer() = default;
			virtual void vdg_mode_update(MonitorInterface &, VDGMode) = 0;
		};

	private:
		std::set<Observer *> m_observers;
	protected:
		MonitorInterface() = default;
	public:
		virtual Part::id_type id() const = 0;
		virtual VDGMode vdg_mode() const = 0;
	protected:
		void vdg_mode_notify(VDGMode p_mode)
			{
				for (auto obs : m_observers)
					obs->vdg_mode_update(*this, p_mode);
			}
	public:
		void attach(Observer & p_observer)
			{ m_observers.insert(&p_observer); }

		void detach(Observer & p_observer)
			{ m_observers.erase(&p_observer); }

		virtual ~MonitorInterface()
			{
				m_observers.clear();
			}

	};

}

#endif

// atom_streambuf.hpp
// Copyright 2016 Steve Palmer

#ifndef ATOM_STREAMBUF_HPP_
#define ATOM_STREAMBUF_HPP_

#include <streambuf>

#include "device.hpp"
#include "memory.hpp"
#include "cpu.hpp"
#include "synchronization_queue.hpp"


namespace Atom
{

    class Streambuf
        : public std::streambuf
    {
        // Types
    public:
        class OSRDCH_Adaptor
            : public Hook
        {
        public:
            class Configurator
                : private NonCopyable
            {
            protected:
                Configurator() = default;
            public:
                virtual ~Configurator() = default;
                virtual const Part::id_type &mcs6502() const = 0;
            };
            // Attributes
        private:
            Streambuf &m_streambuf;
            MCS6502  &m_mcs6502;
            // Methods
        public:
            OSRDCH_Adaptor(const Configurator &, Streambuf &);
            void attach();
        private:
            virtual void terminating();
            virtual int get_byte_hook(word, AccessType);
        };

        class OSWRCH_Adaptor
            : public Hook
        {
        public:
            class Configurator
                : private NonCopyable
            {
            protected:
                Configurator() = default;
            public:
                virtual ~Configurator() = default;
                virtual const Part::id_type &mcs6502() const = 0;
                virtual bool pause_output() const = 0;
            };
            // Attributes
        private:
            Streambuf &m_streambuf;
            MCS6502  &m_mcs6502;
        public:
            bool is_paused;
            // Methods
        public:
            OSWRCH_Adaptor(const Configurator &, Streambuf &);
            void attach();
        private:
            virtual int get_byte_hook(word, AccessType p_at);
        };

        class Configurator
            : public virtual OSRDCH_Adaptor::Configurator
            , public virtual OSWRCH_Adaptor::Configurator
        {
        protected:
            Configurator() = default;
        public:
            virtual ~Configurator() = default;
            virtual const Memory::Configurator *address_space() const = 0;
        };
    public:
        SynchronizationQueue<int_type> put_queue;
        SynchronizationQueue<int_type> get_queue;
        AddressSpace   *m_address_space;
        OSRDCH_Adaptor *m_OSRDCH;
        OSWRCH_Adaptor *m_OSWRCH;

    public:
        explicit Streambuf(const Configurator &);
        void terminating();
        virtual ~Streambuf();

        void reset()
            {
                m_OSWRCH->is_paused = true;
                put_queue.nonblocking_clear();
                get_queue.nonblocking_clear();
            }
        void pause()
            {
                m_OSWRCH->is_paused = true;
            }
        void resume()
            {
                m_OSWRCH->is_paused = false;
            }
        bool is_paused() const
            {
                return m_OSWRCH->is_paused;
            }

        virtual int_type overflow(int_type p_ch);
        virtual int_type underflow();
        virtual int_type uflow();
    };


    class IStream
        : public std::istream
        , public Device
    {
        // Types
    public:
        class Configurator
            : public virtual Device::Configurator
            , public virtual Streambuf::Configurator
        {
            virtual Device *device_factory() const
                { return new Atom::IStream(*this); }
        };

    public:
        explicit IStream(const Configurator &);
    private:
        virtual void terminating();
        virtual ~IStream();
    public:
        virtual void reset()           { dynamic_cast<Atom::Streambuf *>(rdbuf())->reset(); }
        virtual void pause()           { dynamic_cast<Atom::Streambuf *>(rdbuf())->pause(); }
        virtual void resume()          { dynamic_cast<Atom::Streambuf *>(rdbuf())->resume(); }
        virtual bool is_paused() const { return dynamic_cast<Atom::Streambuf *>(rdbuf())->is_paused(); }
    };


    class OStream
        : public std::ostream
        , public Device
    {
        // Types
    public:
        class Configurator
            : public virtual Device::Configurator
            , public virtual Streambuf::Configurator
        {
            virtual Device *device_factory() const
                { return new Atom::OStream(*this); }
        };

    public:
        explicit OStream(const Configurator &);
    private:
        virtual void terminating();
        virtual ~OStream();
    public:
        virtual void reset()           { dynamic_cast<Atom::Streambuf *>(rdbuf())->reset(); }
        virtual void pause()           { dynamic_cast<Atom::Streambuf *>(rdbuf())->pause(); }
        virtual void resume()          { dynamic_cast<Atom::Streambuf *>(rdbuf())->resume(); }
        virtual bool is_paused() const { return dynamic_cast<Atom::Streambuf *>(rdbuf())->is_paused(); }
    };


    class IOStream
        : public std::iostream
        , public Device
    {
        // Types
    public:
        class Configurator
            : public virtual Device::Configurator
            , public virtual Streambuf::Configurator
        {
            virtual Device *device_factory() const
                { return new Atom::IOStream(*this); }
        };

    public:
        explicit IOStream(const Configurator &);
    private:
        virtual void terminating();
        virtual ~IOStream();
    public:
        virtual void reset()           { dynamic_cast<Atom::Streambuf *>(rdbuf())->reset(); }
        virtual void pause()           { dynamic_cast<Atom::Streambuf *>(rdbuf())->pause(); }
        virtual void resume()          { dynamic_cast<Atom::Streambuf *>(rdbuf())->resume(); }
        virtual bool is_paused() const { return dynamic_cast<Atom::Streambuf *>(rdbuf())->is_paused(); }
    };

}

#endif

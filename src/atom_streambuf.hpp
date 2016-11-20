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
            };
            // Attributes
        private:
            Streambuf &streambuf;
            // Methods
        public:
            OSRDCH_Adaptor(const Configurator &, Streambuf &);
            void attach();
        private:
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
                virtual bool pause_output() const = 0;
            };
            // Attributes
        private:
            Streambuf &streambuf;
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
            virtual const Device::Configurator *mcs6502() const = 0;
        };
    public:
        SynchronizationQueue<int_type> put_queue;
        SynchronizationQueue<int_type> get_queue;
        MCS6502        *mcs6502;
        AddressSpace   *address_space;
        OSRDCH_Adaptor *OSRDCH;
        OSWRCH_Adaptor *OSWRCH;

    private:
        Streambuf();
    public:
        explicit Streambuf(const Configurator &);
        void terminating();
        virtual ~Streambuf();

        void reset()
            {
                OSWRCH->is_paused = true;
                put_queue.nonblocking_clear();
                get_queue.nonblocking_clear();
            }
        void pause()
            {
                OSWRCH->is_paused = true;
            }
        void resume()
            {
                OSWRCH->is_paused = false;
            }
        bool is_paused() const
            {
                return OSWRCH->is_paused;
            }

        virtual int_type overflow(int_type p_ch);
        virtual int_type underflow();
        virtual int_type uflow();
    };

    class StreamDevice
        : public Device
    {
    public:
        class Configurator
            : public virtual Device::Configurator
            , public virtual Streambuf::Configurator
        {
        };
    protected:
        Streambuf *streambuf;
    private:
        StreamDevice();
    protected:
        explicit StreamDevice(const Configurator &);
        virtual ~StreamDevice();
    public:
        virtual void reset();
        virtual void pause();
        virtual void resume();
        virtual bool is_paused();
        void unblock();
        virtual void terminating();
    };
    
    class IStream
        : public StreamDevice
        , public std::istream
    {
        // Types
    public:
        class Configurator
            : public virtual StreamDevice::Configurator
        {
            virtual Device *device_factory() const
                { return new IStream(*this); }
        };

    private:
        IStream();
    public:
        explicit IStream(const Configurator &);
    private:
        virtual ~IStream();
    };

    class OStream
        : public StreamDevice
        , public std::ostream
    {
        // Types
    public:
        class Configurator
            : public virtual StreamDevice::Configurator
        {
            virtual Device *device_factory() const
                { return new OStream(*this); }
        };

    private:
        OStream();
    public:
        explicit OStream(const Configurator &);
    private:
        virtual ~OStream();
    };

    class IOStream
        : public StreamDevice
        , public std::iostream
    {
        // Types
    public:
        class Configurator
            : public virtual StreamDevice::Configurator
        {
            virtual Device *device_factory() const
                { return new IOStream(*this); }
        };

    private:
        IOStream();
    public:
        explicit IOStream(const Configurator &);
    private:
        virtual ~IOStream();
    };

}

#endif

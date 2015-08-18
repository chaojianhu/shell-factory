#ifndef CHANNEL_HELPER_H_
#define CHANNEL_HELPER_H_

/* Type traits can be included since they rely exclusively on standard headers and no external linking. */
#include <type_traits>

namespace Pico {

    enum channel_mode
    {
        NO_CHANNEL,
        TCP_CONNECT,
        TCP_LISTEN,
        TCP6_CONNECT,
        TCP6_LISTEN,
        SCTP_CONNECT,
        SCTP_LISTEN,
        SCTP6_CONNECT,
        SCTP6_LISTEN,
        USE_STDOUT,
        USE_STDERR,
    };

    template <enum channel_mode>
    struct ChannelMode;

    #define DEFINE_CHANNEL_MODE(mode, type, dupable)            \
        template<>                                              \
        struct ChannelMode<mode>                                \
        {                                                       \
            typedef type stream_type;                           \
            static constexpr bool dupable_to_stdio = dupable;   \
        };                                                      \

    DEFINE_CHANNEL_MODE(NO_CHANNEL,     void,                   false);
    DEFINE_CHANNEL_MODE(USE_STDOUT,     BiStream<Stream>,       false);
    DEFINE_CHANNEL_MODE(USE_STDERR,     BiStream<Stream>,       false);
    DEFINE_CHANNEL_MODE(TCP_CONNECT,    Network::TcpSocket,     true);
    DEFINE_CHANNEL_MODE(TCP6_CONNECT,   Network::Tcp6Socket,    true);
    DEFINE_CHANNEL_MODE(TCP_LISTEN,     Network::TcpSocket,     true);
    DEFINE_CHANNEL_MODE(TCP6_LISTEN,    Network::Tcp6Socket,    true);
    DEFINE_CHANNEL_MODE(SCTP_CONNECT,   Network::SctpSocket,    true);
    DEFINE_CHANNEL_MODE(SCTP6_CONNECT,  Network::Sctp6Socket,   true);
    DEFINE_CHANNEL_MODE(SCTP_LISTEN,    Network::SctpSocket,    true);
    DEFINE_CHANNEL_MODE(SCTP6_LISTEN,   Network::Sctp6Socket,   true);

    template <enum channel_mode M>
    struct Channel
    {
        static_assert(M != NO_CHANNEL, "Cannot instanciate channel: no mode specified");
        typename ChannelMode<M>::stream_type stm;

        CONSTRUCTOR Channel();

        template <enum Network::AddressType T>
        CONSTRUCTOR Channel(Network::Address<T> addr, uint16_t port);

        METHOD Channel& recv(void *buf, size_t count) {
            stm.read(buf, count);
            return *this;
        }

        METHOD Channel& recv(Memory::Buffer const& buffer) {
            return recv(buffer.pointer(), buffer.size());
        }

        METHOD Channel& send(const void *buf, size_t count) {
            stm.write(buf, count);
            return *this;
        }

        METHOD Channel& send(Memory::Buffer const& buffer) {
            return send(buffer.pointer(), buffer.size());
        }

        METHOD void dup_to_stdio() {
            if ( ChannelMode<M>::dupable_to_stdio )
            {
                Stream std_in = Stream::standard_input();
                Stream std_out = Stream::standard_output();

                stm.duplicate(std_in, std_out);
            }
        }
    };

    template<>
    CONSTRUCTOR
    Channel<USE_STDOUT>::Channel() :
        stm(Stream::standard_input(), Stream::standard_output()) {}

    template<>
    CONSTRUCTOR
    Channel<USE_STDERR>::Channel() :
        stm(Stream::standard_input(), Stream::standard_error()) {}

    template <>
    template <enum Network::AddressType T>
    CONSTRUCTOR
    Channel<TCP_CONNECT>::Channel(Network::Address<T> addr, uint16_t port) {
        static_assert(T == Network::IPV4, "TCP_CONNECT requires an IPV4 address.");
        stm.connect(addr, port); 
    }

    template <>
    template <enum Network::AddressType T>
    CONSTRUCTOR
    Channel<TCP6_CONNECT>::Channel(Network::Address<T> addr, uint16_t port) {
        static_assert(T == Network::IPV6, "TCP6_CONNECT requires an IPV6 address.");
        stm.connect(addr, port); 
    }

    template <>
    template <enum Network::AddressType T>
    CONSTRUCTOR
    Channel<TCP_LISTEN>::Channel(Network::Address<T> addr, uint16_t port) :
        stm(Network::SocketServer<Network::TcpSocket>::start(addr, port, Options::reuse_addr, Options::fork_on_accept))
    {
        static_assert(T == Network::IPV4, "TCP_LISTEN requires an IPV4 address.");
    }

    template <>
    template <enum Network::AddressType T>
    CONSTRUCTOR
    Channel<TCP6_LISTEN>::Channel(Network::Address<T> addr, uint16_t port) :
        stm(Network::SocketServer<Network::Tcp6Socket>::start(addr, port, Options::reuse_addr, Options::fork_on_accept))
    {
        static_assert(T == Network::IPV6, "TCP6_LISTEN requires an IPV6 address.");
    }

    template <>
    template <enum Network::AddressType T>
    CONSTRUCTOR
    Channel<SCTP_CONNECT>::Channel(Network::Address<T> addr, uint16_t port) {
        static_assert(T == Network::IPV4, "SCTP_CONNECT requires an IPV4 address.");
        stm.connect(addr, port); 
    }

    template <>
    template <enum Network::AddressType T>
    CONSTRUCTOR
    Channel<SCTP6_CONNECT>::Channel(Network::Address<T> addr, uint16_t port) {
        static_assert(T == Network::IPV6, "SCTP6_CONNECT requires an IPV6 address.");
        stm.connect(addr, port); 
    }

    template <>
    template <enum Network::AddressType T>
    CONSTRUCTOR
    Channel<SCTP_LISTEN>::Channel(Network::Address<T> addr, uint16_t port) :
        stm(Network::SocketServer<Network::SctpSocket>::start(addr, port, Options::reuse_addr, Options::fork_on_accept))
    {
        static_assert(T == Network::IPV4, "SCTP_LISTEN requires an IPV4 address.");
    }

    template <>
    template <enum Network::AddressType T>
    CONSTRUCTOR
    Channel<SCTP6_LISTEN>::Channel(Network::Address<T> addr, uint16_t port) :
        stm(Network::SocketServer<Network::Sctp6Socket>::start(addr, port, Options::reuse_addr, Options::fork_on_accept))
    {
        static_assert(T == Network::IPV6, "SCTP6_LISTEN requires an IPV6 address.");
    }
}

namespace Options {

    /*
     * Channel parameter defaults to NO_CHANNEL.
     */
    #ifndef CHANNEL
    #define CHANNEL NO_CHANNEL
    #endif

    /*
     * HOST parameter.
     * Used for socket channels.
     */
    #ifndef HOST
    #define HOST            0,0,0,0
    #endif

    /*
     * PORT parameter.
     * Used for socket channels.
     */
    #ifndef PORT
    #define PORT            0
    #endif

    FUNCTION auto channel()
    {
        using Mode = ChannelMode<CHANNEL>;

        if ( std::is_base_of<Network::Socket, Mode::stream_type>::value )
        {
            uint16_t port = PORT;
            auto address = Network::ip_address_from_bytes(HOST);

            return Channel<CHANNEL>(address, port); 
        }
        else
            return Channel<CHANNEL>();
    }
}

#endif
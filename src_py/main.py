import asyncio

UDP_SRC_IPV4_ADDR = "0.0.0.0"
UDP_SRC_PORT = 8888

class UdpDataReceiver(asyncio.Protocol):
    def __init__(self, on_con_lost):
        self.on_con_lost = on_con_lost

    def datagram_received(self, data, _addr):
        message = data.decode()
        print(f"received message:\n{message}")

    def connection_lost(self, _exc):
        self.on_con_lost.set_result(True)


async def eth_udp_rx():
    loop = asyncio.get_running_loop()

    on_con_lost = loop.create_future()

    transport, _ = await loop.create_datagram_endpoint(
        lambda: UdpDataReceiver(on_con_lost),
        local_addr=(UDP_SRC_IPV4_ADDR, UDP_SRC_PORT)
    )

    try:
        await on_con_lost
    finally:
        transport.close()

asyncio.run(eth_udp_rx())

#!/usr/bin/env python3
# bidirectional_serial_http.py
# Python 3.8+
# Requires: aiohttp, pyserial-asyncio
# pip install aiohttp pyserial-asyncio

import asyncio
import argparse
import logging
from aiohttp import web, ClientSession
import serial_asyncio

logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s: %(message)s")


async def serial_reader_task(reader, post_url, client_session, newline=b"\n"):
    """シリアルから行を読み、HTTP POSTで localhost の URL に送る"""
    while True:
        try:
            line = await reader.readuntil(newline)  # newlineで区切る
        except asyncio.IncompleteReadError as e:
            if e.partial:
                line = e.partial
            else:
                logging.info("serial: EOF")
                break
        except Exception as e:
            logging.exception("serial read error")
            await asyncio.sleep(1)
            continue

        # raw bytes のまま投げる（必要ならデコードして加工しても良い）
        logging.info("recv from serial: %r", line)
        try:
            async with client_session.post(post_url, data=line) as resp:
                logging.info("posted to %s -> %s", post_url, resp.status)
        except Exception:
            logging.exception("failed to post serial -> http")


async def handle_http_to_serial(request):
    """HTTP POST handler: body をそのままシリアルへ書く"""
    app = request.app
    writer = app["serial_writer"]
    if writer is None:
        return web.Response(status=503, text="serial not connected")

    data = await request.read()  # raw body
    if not data:
        return web.Response(status=400, text="empty body")

    try:
        # write and flush
        writer.write(data)
        # ensure newline (optionally)
        # writer.write(b"\n")
        await writer.drain()
        logging.info("wrote to serial: %r", data)
        return web.Response(status=200, text="ok")
    except Exception:
        logging.exception("failed to write to serial")
        return web.Response(status=500, text="failed to write to serial")


async def start_serial(app, port, baudrate, newline=b"\n"):
    """シリアル接続を開き、reader/writer を app に格納。reader タスク開始"""
    try:
        reader, writer = await serial_asyncio.open_serial_connection(url=port, baudrate=baudrate)
    except Exception:
        logging.exception("failed to open serial port %s", port)
        raise

    app["serial_writer"] = writer
    logging.info("serial opened %s @ %d", port, baudrate)

    # HTTPポスト先（シリアル->HTTP）
    post_url = app["post_url"]
    client_session = app["client_session"]
    app["serial_task"] = asyncio.create_task(serial_reader_task(reader, post_url, client_session, newline=newline))


async def stop_serial(app):
    task = app.get("serial_task")
    if task:
        task.cancel()
        try:
            await task
        except asyncio.CancelledError:
            pass

    writer = app.get("serial_writer")
    if writer:
        try:
            writer.close()
            await writer.wait_closed()
        except Exception:
            pass
    logging.info("serial stopped")


async def on_startup(app):
    await start_serial(app, app["serial_port"], app["baudrate"], newline=app["newline"])


async def on_cleanup(app):
    await stop_serial(app)
    cs = app.get("client_session")
    if cs:
        await cs.close()


def make_app(args):
    app = web.Application()
    app.add_routes([web.post('/to-serial', handle_http_to_serial)])
    # store config into app
    app["serial_port"] = args.port
    app["baudrate"] = args.baud
    app["post_url"] = args.post_url
    app["newline"] = args.newline.encode() if isinstance(args.newline, str) else args.newline
    app["serial_writer"] = None
    app["serial_task"] = None
    app["client_session"] = ClientSession()
    app.on_startup.append(on_startup)
    app.on_cleanup.append(on_cleanup)
    return app


def parse_args():
    p = argparse.ArgumentParser(description="Bidirectional bridge between serial and HTTP (localhost)")
    p.add_argument("--port", "-p", required=True, help="Serial device (e.g. /dev/ttyUSB0 or COM3)")
    p.add_argument("--baud", "-b", type=int, default=9600, help="Serial baudrate")
    p.add_argument("--http-host", default="127.0.0.1", help="Host for HTTP server (to receive -> serial)")
    p.add_argument("--http-port", type=int, default=8000, help="Port for HTTP server (to receive -> serial)")
    p.add_argument("--post-url", default="http://127.0.0.1:9000/from-serial", help="URL to POST serial->http messages")
    p.add_argument("--newline", default="\n", help="Line delimiter used for serial reading (default: LF)")
    return p.parse_args()


def main():
    args = parse_args()
    app = make_app(args)
    logging.info("HTTP server will listen on http://%s:%d", args.http_host, args.http_port)
    web.run_app(app, host=args.http_host, port=args.http_port)


if __name__ == "__main__":
    main()

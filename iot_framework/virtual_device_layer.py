import logging
import asyncio
import json

# Appending aiohttp server and VDL structure

try:
    from aiohttp import web
    logging.info("aiohttp imported successfully.")
except ImportError:
    logging.error("aiohttp library not found. The HTTP server cannot be started. Install with: pip install aiohttp")
    web = None

from typing import Dict, Any, List, Optional, Callable
from dataclasses import dataclass, field
from enum import Enum

class ProtocolType(Enum):
    HTTP = "http"
    MQTT = "mqtt"

@dataclass
class DeviceDescriptor:
    id: str
    name: str
    protocol: ProtocolType
    endpoint: str
    port: Optional[int] = None
    status: Dict[str, Any] = field(default_factory=lambda: {"state": "unknown"})

    def to_dict(self):
        return {
            "id": self.id,
            "name": self.name,
            "protocol": self.protocol.value,
            "endpoint": self.endpoint,
            "port": self.port,
            "status": self.status,
        }

class VirtualDeviceLayer:
    def __init__(self):
        self.devices: Dict[str, DeviceDescriptor] = {}
        logging.info("VirtualDeviceLayer initialized.")

    async def discover_and_register_devices(self):
        discovered_devices = [
            DeviceDescriptor(id="light001", name="Living Room Light", protocol=ProtocolType.HTTP, endpoint="192.168.1.100", port=8080),
            DeviceDescriptor(id="thermostat001", name="Office Thermostat", protocol=ProtocolType.MQTT, endpoint="mqtt.example.com", port=1883)
        ]
        for dev in discovered_devices:
            if dev.id not in self.devices:
                self.devices[dev.id] = dev
                logging.info(f"Registered device: {dev.name} ({dev.id})")
        await asyncio.sleep(0.1)
        logging.info(f"Finished registering {len(self.devices)} devices.")

    async def get_device_status(self, device_id: str) -> Optional[Dict[str, Any]]:
        device = self.devices.get(device_id)
        if device:
            logging.info(f"Getting status for {device_id}: {device.status}")
            return device.status
        logging.warning(f"Device {device_id} not found for get_status.")
        return None

    async def send_device_command(self, device_id: str, command: str, params: Optional[Dict[str, Any]] = None) -> Optional[Dict[str, Any]]:
        device = self.devices.get(device_id)
        if device:
            logging.info(f"Sending command '{command}' to {device_id} with params: {params}")
            if command == "turn_on":
                device.status = {"state": "on"}
                return {"status": "success", "message": f"{device.name} turned on."}
            elif command == "turn_off":
                device.status = {"state": "off"}
                return {"status": "success", "message": f"{device.name} turned off."}
            else:
                return {"status": "failed", "message": "Unknown command"}
        logging.warning(f"Device {device_id} not found for send_command.")
        return None

    def get_all_devices_info(self) -> List[Dict[str, Any]]:
        return [dev.to_dict() for dev in self.devices.values()]

    def get_device_info_by_id(self, device_id: str) -> Optional[Dict[str, Any]]:
        device = self.devices.get(device_id)
        return device.to_dict() if device else None

    async def cleanup(self):
        logging.info("VirtualDeviceLayer cleaning up...")
        self.devices.clear()
        logging.info("All devices unregistered.")

async def handle_get_devices(request: web.Request) -> web.Response:
    vdl: VirtualDeviceLayer = request.app['vdl']
    devices_info = vdl.get_all_devices_info()
    return web.json_response(devices_info)

async def handle_get_device_status(request: web.Request) -> web.Response:
    vdl: VirtualDeviceLayer = request.app['vdl']
    device_id = request.match_info['device_id']
    device_info = vdl.get_device_info_by_id(device_id)
    if not device_info:
        return web.json_response({"error": "Device not found"}, status=404)
    status = await vdl.get_device_status(device_id)
    if status is None:
        return web.json_response({"error": f"Could not get status for {device_id}"}, status=500)
    return web.json_response(status)

async def handle_post_device_command(request: web.Request) -> web.Response:
    vdl: VirtualDeviceLayer = request.app['vdl']
    device_id = request.match_info['device_id']
    device_info = vdl.get_device_info_by_id(device_id)
    if not device_info:
        return web.json_response({"error": "Device not found"}, status=404)
    try:
        data = await request.json()
    except json.JSONDecodeError: # Changed from json.decoder.JSONDecodeError
        return web.json_response({"error": "Invalid JSON payload"}, status=400)
    command = data.get("command")
    params = data.get("params")
    if not command:
        return web.json_response({"error": "Missing 'command' in payload"}, status=400)
    result = await vdl.send_device_command(device_id, command, params)
    if result is None or (isinstance(result, dict) and result.get("status") == "failed"):
        return web.json_response({"error": f"Command failed for {device_id}", "details": result}, status=500)
    return web.json_response(result)

async def app_startup(app: web.Application):
    logging.info("Application starting up...")
    vdl = VirtualDeviceLayer()
    await vdl.discover_and_register_devices()
    app['vdl'] = vdl
    logging.info("VDL setup and device registration complete for startup.")

async def app_cleanup(app: web.Application):
    logging.info("Application shutting down...")
    vdl: VirtualDeviceLayer = app['vdl']
    await vdl.cleanup()
    logging.info("VDL cleanup complete.")

def main_server():
    if not web:
        logging.error("aiohttp is not installed. Cannot run API server.")
        return

    app = web.Application()
    app.router.add_get("/devices", handle_get_devices)
    app.router.add_get("/devices/{device_id}/status", handle_get_device_status)
    app.router.add_post("/devices/{device_id}/command", handle_post_device_command)

    app.on_startup.append(app_startup)
    app.on_cleanup.append(app_cleanup)

    host = '0.0.0.0'
    port = 8080
    logging.info(f"Starting Virtual Device Layer API server on http://{host}:{port}")
    web.run_app(app, host=host, port=port)

if __name__ == "__main__":
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
        handlers=[logging.StreamHandler()],
        force=True # force=True to allow re-configuration if already set by minimal script
    )
    logging.info("Attempting to start main_server if aiohttp is available.")
    main_server()

import asyncio
import psutil
from bleak import BleakClient, BleakScanner
import json
import requests
from requests.auth import HTTPBasicAuth

# ==== Config HTTP LibreHardwareMonitor ====
URL = "http://192.168.100.l10/data.json"
DEVICE_NAME = "ESP32_TaskManager"
AUTH = HTTPBasicAuth('admin', 'lhm')  # sesuaikan kalau pakai auth

# ==== Ambil Core Average dari LHMonitor ====
def get_lhm_core_average():
    def search_core_average(node):
        # jika node dict
        if isinstance(node, dict):
            if node.get("Text") == "Core Average":
                value = node.get("Value")
                if value is None:
                    return 0.0
                try:
                    value_str = str(value).replace("°C", "").replace(",", ".").strip()
                    return round(float(value_str), 1)
                except ValueError:
                    return 0.0
            # telusuri children jika ada
            for child in node.get("Children", []):
                result = search_core_average(child)
                if result is not None:
                    return result
        # jika node list
        elif isinstance(node, list):
            for item in node:
                result = search_core_average(item)
                if result is not None:
                    return result
        return None

    try:
        r = requests.get(URL, timeout=5, auth=AUTH)
        if r.status_code != 200:
            print(f"[WARN] LHMonitor HTTP status: {r.status_code}")
            return 0.0
        data = r.json()
        result = search_core_average(data)
        if result is None:
            print("[WARN] Core Average not found in LHMonitor data")
            return 0.0
        return result
    except Exception as e:
        print("[ERROR] Fetching Core Average failed:", e)
        return 0.0

# ==== Scan BLE device dengan retry ====
async def find_device(name, retries=5):
    for attempt in range(retries):
        try:
            print(f"[INFO] Scanning for {name}... (Attempt {attempt+1}/{retries})")
            devices = await BleakScanner.discover(timeout=5.0)
            for d in devices:
                if d.name == name:
                    print(f"[FOUND] {name} - {d.address}")
                    return d.address
        except OSError as e:
            print("[WARN] BLE scan error:", e)
        await asyncio.sleep(2)
    print("[ERROR] Device not found after retries!")
    return None

# ==== Kirim stats ke ESP32 ====
async def send_stats(address):
    while True:
        try:
            async with BleakClient(address) as client:
                print("[CONNECTED] to ESP32")
                while True:
                    cpu = psutil.cpu_percent(interval=1)
                    mem = psutil.virtual_memory()
                    disk = psutil.disk_usage('/')
                    battery = psutil.sensors_battery()
                    temp = get_lhm_core_average()

                    stats = {
                        "cpu": cpu,
                        "ram_used": mem.used // (1024 * 1024),
                        "ram_total": mem.total // (1024 * 1024),
                        "disk_used": disk.used // (1024 * 1024 * 1024),
                        "disk_total": disk.total // (1024 * 1024 * 1024),
                        "battery": battery.percent if battery else 0,
                        "temperature": temp
                    }

                    msg = json.dumps(stats)
                    await client.write_gatt_char("abcdef12-3456-7890-abcd-ef1234567890", msg.encode())
                    print("Sent:", msg)
        except Exception as e:
            print("[WARN] BLE disconnected or error:", e)
            print("Retrying in 5s...")
            await asyncio.sleep(5)

# ==== Main ====
async def main():
    address = await find_device(DEVICE_NAME)
    if address:
        await send_stats(address)
    else:
        print("[ERROR] Could not find device. Exiting.")

if __name__ == "__main__":
    asyncio.run(main())

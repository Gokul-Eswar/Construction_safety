import pytest
import requests
import time
import subprocess
import json
import os

# This test requires the system to be running (e.g., via docker-compose)
# It verifies that the Web API reflects the live state of the C++ Engine.

BASE_URL = "http://localhost:3001/api"

def is_system_up():
    try:
        response = requests.get(f"{BASE_URL}/stats", timeout=2)
        return response.status_code == 200
    except:
        return False

@pytest.mark.skipif(not is_system_up(), reason="System must be running to perform E2E tests")
def test_web_backend_connectivity():
    """Verify Web Backend is reachable."""
    response = requests.get(f"{BASE_URL}/stats")
    assert response.status_code == 200
    data = response.json()
    assert "system_status" in data

@pytest.mark.skipif(not is_system_up(), reason="System must be running to perform E2E tests")
def test_engine_heartbeat_via_api():
    """Verify that the C++ Engine's heartbeat is reaching the Web Backend."""
    # The C++ engine writes to heartbeat.json AND publishes to MQTT.
    # The Web API /stats endpoint should show 'online'.
    
    # Wait up to 10 seconds for a fresh heartbeat
    for _ in range(10):
        response = requests.get(f"{BASE_URL}/stats")
        data = response.json()
        if data.get("system_status") == "online":
            return
        time.sleep(1)
    
    pytest.fail("C++ Engine heartbeat not detected by Web Backend within timeout.")

@pytest.mark.skipif(not is_system_up(), reason="System must be running to perform E2E tests")
def test_config_retrieval():
    """Verify that the system configuration is accessible via API."""
    response = requests.get(f"{BASE_URL}/config")
    assert response.status_code == 200
    config = response.json()
    assert "streams" in config or "zones" in config

def test_heartbeat_file_integrity():
    """Verify that the C++ engine is writing the local heartbeat file."""
    # This assumes the test runs in the same environment as the engine (or same mount)
    hb_path = "heartbeat.json"
    if not os.path.exists(hb_path):
        pytest.skip("heartbeat.json not found in local path (might be inside container)")
        
    with open(hb_path, "r") as f:
        data = json.load(f)
        assert "timestamp" in data
        assert data["status"] == "running"
        
        # Verify freshness (within 30 seconds)
        current_time = time.time()
        assert abs(current_time - data["timestamp"]) < 30

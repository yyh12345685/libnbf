import pathlib, subprocess, tempfile, time, os, signal, socket, struct, sys
build = pathlib.Path(sys.argv[1]).resolve()
source = pathlib.Path(__file__).resolve().parents[2]
base = build / "examples/coroutine_stress_testing/bin"
with tempfile.TemporaryDirectory(prefix="libnbf-regression-") as d:
    root = pathlib.Path(d)
    conf = source / "examples/coroutine_stress_testing/conf"
    ports = []
    for _ in range(3):
        s = socket.socket(); s.bind(("127.0.0.1", 0)); ports.append(s.getsockname()[1]); s.close()
    hp, rp, cp = ports
    for name in ["stress_testing_server.conf", "stress_testing_client.conf", "router.conf"]:
        text = (conf/name).read_text().replace(":9090", ":%d"%hp).replace(":9091", ":%d"%rp).replace(":12345", ":%d"%cp)
        text = text.replace("../conf/router.conf", str(root/"router.conf"))
        text = text.replace("../log/", str(root)+"/")
        text = text.replace("single_addr_connect_count = 500", "single_addr_connect_count = 12").replace("single_addr_connect_count = 300", "single_addr_connect_count = 12")
        text = text.replace("heartbeat_ms = 3000", "heartbeat_ms = 200")
        (root/name).write_text(text)
    processes = []
    def start(role):
        log = root/(role+".logger")
        log.write_text("log4cplus.rootLogger=INFO, R\nlog4cplus.appender.R=log4cplus::FileAppender\nlog4cplus.appender.R.File="+str(root/(role+".log"))+"\nlog4cplus.appender.R.layout=log4cplus::PatternLayout\nlog4cplus.appender.R.layout.ConversionPattern=%m%n\n")
        proc = subprocess.Popen([str(base/("stress_"+role+"_test")), "-c", str(root/("stress_testing_"+role+".conf")), "-l",str(log)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, env=dict(os.environ, LIBNBF_STRESS_INTERVAL_US="1000"))
        processes.append(proc); return proc
    try:
        server = start("server"); time.sleep(1)
        before = len(os.listdir("/proc/%d/fd"%server.pid))
        for _ in range(100):
            with socket.create_connection(("127.0.0.1",hp),2) as s:
                request=b"POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\n\r\nhello"
                for part in [request[:23],request[23:-2],request[-2:]]: s.sendall(part); time.sleep(.002)
                s.settimeout(2); assert b"200 OK" in s.recv(4096)
        time.sleep(1)
        after = len(os.listdir("/proc/%d/fd"%server.pid)); assert after == before,(before,after)
        with socket.create_connection(("127.0.0.1",rp),2) as s:
            body=b"split rapid payload"; frame=struct.pack("!HBBIII",11111,0,0,0,123,len(body)+16)+body
            s.sendall(frame[:18]);time.sleep(.02);s.sendall(frame[18:]);s.settimeout(2);assert s.recv(4096)
        client=start("client");time.sleep(35)
        assert client.poll() is None and server.poll() is None
        reports="".join(x.read_text() for x in root.glob("monitor_test_client.log*"))
        print(reports)
        assert "http_test_client.OK" in reports and "rapid_test_client.OK" in reports
        assert ".ERR" not in reports
        log=(root/"client.log").read_text()
        assert "HeartBeat time out" not in log and "Broken pipe" not in log
        print("fragmented sockets, FD recovery and repeated HTTP/Rapid heartbeats passed")
    finally:
        for proc in reversed(processes):
            if proc.poll() is None: proc.terminate()
        for proc in processes:
            try:proc.wait(timeout=12)
            except subprocess.TimeoutExpired:proc.kill();proc.wait()

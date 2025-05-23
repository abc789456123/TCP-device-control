server 환경 빌드

1. 빌드:
    
    ```bash
    make clean; make
    ```
    
2. 데몬 서버 실행:
    
    ```bash
    ./server
    ```
    
3. 로그 확인:
    
    ```bash
    journalctl -t tcp-server -f
    ```
    

- 로그 내용 초기화
    
    ```bash
    sudo journalctl --rotate
    sudo journalctl --vacuum-time=1s
    ```
    
- 실행 중인 서버 종료
    
    ```bash
    sudo pkill server
    ```
    

---

client 빌드

1. 빌드
    
    ```bash
    cc client.c -o client
    ```
    
2. 실행
    
    ```bash
    ./client <server's IP address>
    ```
    

---

주의 사항

build 이후에 client 실행 파일을 제외한 다른 파일들의 위치를 옮기면 안됩니다.

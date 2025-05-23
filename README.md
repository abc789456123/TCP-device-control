코드 가이드

1. 빌드:
    
    ```bash
    make clean; make
    ```
    
2. 데몬 서버 실행:
    
    ```bash
    ./server
    ```

3. 클라이언트 실행
    
    ```bash
    ./client <server's address>
    ```
    
4. 로그 확인:
    
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

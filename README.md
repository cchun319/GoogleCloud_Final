# PennCloud
- [project](project.pdf)  
- [report](report.pdf)

## Architect


## Use
1. bash make in directory: ./frontendServer ./kvstore ./kvstore/master ./webmail  
2. initialize backend according to ./kvstore/serverlist.txt, note the first number is node ID and the second is replicate ID  
```bash
./kvstore/master/masterserver -v ../serverlist.txt
./kvstore/dataserver -p ./node/0/0 -v ./serverlist.txt 0 0
./kvstore/dataserver -p ./node/0/1 -v ./serverlist.txt 0 1
```
3. start frontend according to ./frontendServer/servers.txt
```bash
./frontendServer/load_balancer servers.txt  
./frontendServer/frontend -v -p 8080 
./frontendServer/frontend -v -p 8081 
./frontendServer/frontend -v -p 8082 
./frontendServer/frontend -v -p 8083 
./frontendServer/frontend -v -p 8084 
```  
4. run SMTP server with masterserver address
```bash
./webmail/server -v 127.0.0.1:10000
```
5. open a browser to access localhost frontend, use PennCloud pages to store files, send emails and keep these data distributed

## Technologies
C++

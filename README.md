# PennCloud
- [Brief](#project-brief)  
- [Use](#use)

## Project Brief
PennCloud is a C++ system having complete front-end and back-end design<br/>
The system supports<br/>
:key:user sign-up/log-in<br/>
:email:mail write/send<br/>
:file_folder:file upload/download<br/> 
:balance_scale:load balane<br/> 
:floppy_disk:backup log<br/> 
:paperclip:fault tolerance<br/>  


## Architect
<img src="/struct.png" width="1000" height="500"/> <!-- image-->

## Use
1. Go to the directories: ./frontendServer ./kvstore ./kvstore/master ./webmail and do: 
```bash 
make
```
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

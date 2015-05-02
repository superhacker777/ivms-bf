iVMS-bd — The Webcam Bruteforcer
---

A simple webcam bruteforcer written based Hikvision SDK. WOW, much alpha, very bug, pls help.

## Usage
Command to start bruteforce in 265 threads with IPs list stored in file "ips_file":

    $ ivms-bf -t 265 -i ips_file 2> /dev/null

Yeah, you'll probably want to ignore STDERR messages because they're very noisy. It isn't my fault, though. It is somewhere in SDK lib.

__Passwords__ must be stored in file called "passwords", the same for __logins__ and "logins" file. It's hardcoded now.

## Bugs
There's a lot of noise in STDERR about error 115. They're all coming from SDK library. I'm not sure about what to do with this. I'm going to patch the library.
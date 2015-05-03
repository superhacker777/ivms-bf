iVMS-bd — The Webcam Bruteforcer
---

A simple webcam bruteforcer based on Hikvision SDK. WOW, much alpha, very bug, pls help.


## Building

In most cases, you need this:

  $ make

This includes patched SDK lib so you don't get a lot of useless debug warnings if your STDERR. But if you want to use an original library for some reason, you should make it like this:

  $ make vanilla


## Platform support

I only care about Linux support now, so there's no way of making it work on Windows. There's a big __LATER__ here.


## Usage

Command to start bruteforce in 265 threads with IPs list stored in file "ips_file":

    $ ivms-bf -t 265 -i ips_file 2> /dev/null

Yeah, you'll probably want to ignore STDERR messages because they're very noisy. It isn't my fault, though. It is somewhere in SDK lib.

__UPD__: Now I have this library patched. Make with:

    $ make silent

You could also pass an IP list as an argument, which is helpful when you want to check a short list of cameras:

    $ ivms-bf 192.168.0.1 ...

__Passwords__ must be stored in file called "passwords", the same for __logins__ and "logins" file. It'll be hardcoded for a while.


## Bugs
There may be some broken output. I didn't really know that STDOUT isn't thread-safe. I'm not that smart, uh. I'm going to fix it laterrrrrrr.
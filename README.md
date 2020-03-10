# Soroban in PRUG Terakoya Network System

Soroban is an open source software I inherited in 1992 and maintained until c. 1995. It is a legacy system, and this repository is for archival purposes only. There may be a minor update to the historical notes only.

## BACKGROUND

In the world I was in 1990, almost nobody knew the Internet. There was minimal Internet availability outside of computer-related research labs and university computer labs, and I wasn't old enough to have access to any such things. Most people were using a 4800 or 9600 baud modem on a regular phone line or 64kbps ISDN lines. The bulk of messages were transmitted and relayed by the store-and-forward process in batches, like UUCP, several times a day. Some of us had IP connection in wide area networks and used SMTP and UUCP over IP, but that was not very common.

There was a thing called USENET. Functionally, it's like a cross between Slack and online forums, but all text-based. It was a very functional closed group communication tool in institutions and also an open global discussion system. The messages were processed in batches, relayed via UUCP, or other protocols. There was a very popular news server system called Bnews, which was being replaced by Cnews around that time. They were excellent systems but only ran on UNIX systems, making it inaccessible to users of personal computers.

Packet Radio Users' Group (PRUG) started building always-on IP networks using amateur radio and various pieces of interface hardware in Japan, since 1985. Most of the network nodes were running on MS-DOS version 3, 8086 or 80286, using KA9Q TCP-IP protocol engine (which also handled SMTP and remote login). Members of PRUG developed a mail user agent program (oimo), **USENET news engine (soroban),** and user agent applications (yomikaki) in the late 1980s. I was "recruited" into this group and "ordered" by the senior members to take over this soroban software, so I made many improvements, including a few major restructuring of the software, and maintained it for about three years.

Many of us bought UNIX workstations like Sun3, Sun4, SparkStation, or Sony equivalent NEWS series and phased out Microsoft operating systems altogether. Some others built so-called PC-UNIX using NetBSD, FreeBSD, Linux, etc. on 80386 computers. In a way, increased access to the real UNIX system made Terakoya system obsolete in the 1990s.

## Soroban software

The entire software is written in ANSI C of that time, and in a way that I would not write today. Many parts of the code (and my primitive command of English language) are embarrassing, but I decided to publish the snapshot from 22 May 1993. I used mostly Turbo C/C++, Borland C/C++, and LSI-C. LSI-C produced the most elegant assembly codes and smaller executables, but Borland compilers had better standard IO library with effective buffering, so those ran faster. As I inherited, the early versions of soroban were compiled in the compact model (16-bit addressing for instruction, 20-bit addressing for data), but I rewrote parts of the program to make it compile in the small model (16-bit addressing for both) in later versions. It's embarrassing to admit it, but there are places where int/short is assumed 16-bit (e.g., hash function), and where the NULL pointer is treated as if numeric zero (both of these were always true on MS-DOS version 3 and a lot of codes were written back then), so beware!

USENET network had many redundant connections, so it is not uncommon to receive the same article via different paths. So, it is essential to check for duplicate articles before storing them. This required soroban to have a hash table on disk, which was the major performance bottleneck (many run this system on 2HD floppy disks). If I were to rewrite this system today, Ruby or Python and SQLite will do in far fewer lines of code and will run on the least powerful computer in common use today.

Throughout the source code and documentation, you see a strong influence from BSD UNIX. As they said in hip hop songs, cool kids used 4.2 and 4.3 BSD UNIX, not Microsoft or System V.

The documentation is written in a nroff-like terminal-based typesetter called ntf, a free software made by 新島 智之 . There was no nroff/troff compatible system that could handle Japanese character codes and run on MS-DOS back then. Some comments and most documents are written in the Japanese language.

The Japanese text is encoded ironically in Microsoft Kanji code or so-called shift JIS code. I will replace the text with UTF-8 converted version and re-commit.

## Dentaku software

One significant change I made to soroban after I inherited the project was to limit soroban to the receiving and storage management functions and separated the forwarding function so that each part could be run on cron at different timing or frequency. See **Dentaku** in a separate repository.

## OBSOLETE INFO
Many pieces of information in this repository are absolutely obsolete. None of the email addresses and numeric IP addresses is reachable today.
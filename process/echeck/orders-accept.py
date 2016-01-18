#!/usr/bin/env python
# -*- coding: iso-8859-1 -*-

from email.Utils import parseaddr
from email.Parser import Parser
from os import mkdir, rename, stat, utime, unlink, symlink
from os.path import exists
from re import compile, IGNORECASE
from stat import ST_MTIME
from string import upper, split, replace
from syslog import openlog, syslog, closelog
from sys import argv, stdin, exit
from time import ctime, sleep, time
from socket import gethostname
from rfc822 import parsedate_tz, mktime_tz
prefix = "turn-"
hostname = gethostname()

def unlock_file(filename):
    try:
        unlink(filename+".lock")
    except:
        print "could not unlock %s.lock, file not found" % filename

def lock_file(filename):
    i = 0
    wait = 1
    if not exists(filename):
        file=open(filename, "w")
        file.close()
    while True:
        try:
            symlink(filename, filename+".lock")
            return
        except:
            i = i+1
            if i == 5: unlock_file(filename)
            sleep(wait)
            wait = wait*2

emails = {
 "hse" : "Eressea Wochenende <hse-server@eressea.de>",
 "eressea" : "Eressea Server <eressea-server@eressea.de>",
 "tutorial" : "Eressea Tutorial <tutorial@eressea.de>",
 "test" : "Eressea Server <eressea-server@eressea.de>",
}

messages = {
        "multipart-en" : 
                "ERROR: The orders you sent contain no plaintext. " \
                "The Eressea server cannot process orders containing HTML " \
                "or invalid attachments, which are the reasons why this " \
                "usually happens. Please change the settings of your mail " \
                "software and re-send the orders.",

        "multipart-de" : 
                "FEHLER: Die von dir eingeschickte Mail enthält keinen " \
                "Text. Evtl. hast Du den Zug als HTML oder als anderweitig " \
                "ungültig formatierte Mail ingeschickt. Wir können ihn " \
                "deshalb nicht berücksichtigen. Schicke den Zug nochmals " \
                "als reinen Text ohne Formatierungen ein.",

        "maildate-de":
                "Es erreichte uns bereits ein Zug mit einem späteren " \
                "Absendedatum (%s > %s). Entweder ist deine " \
                "Systemzeit verstellt, oder ein Zug hat einen anderen Zug von " \
                "dir auf dem Transportweg überholt. Entscheidend für die " \
                "Auswertungsreihenfolge ist das Absendedatum, d.h. der Date:-Header " \
                "deiner Mail.",

        "maildate-en":
                "The server already received an order file that was sent at a later " \
                "date (%s > %s). Either your system clock is wrong, or two messages have " \
                "overtaken each other on the way to the server. The order of " \
                "execution on the server is always according to the Date: header in " \
                "your mail.",

        "nodate-en":
                "Your message did not contain a valid Date: header in accordance with RFC2822.",

        "nodate-de":
                "Deine Nachricht enthielt keinen gueltigen Date: header nach RFC2822.",

        "error-de":
                "Fehler",

        "error-en":
                "Error",

        "warning-de":
                "Warnung",

        "warning-en":
                "Warning",
                
        "subject-de":
                "Befehle angekommen",
                
        "subject-en":
                "orders received"
}

# base directory for all your games:
rootdir = "/home/eressea"
orderbase = "orders.dir"
sendmail = True
# maximum number of reports per sender:
maxfiles = 20
# write headers to file?
writeheaders = True
# reject all html email?
rejecthtml = True

# return 1 if addr is a valid email address
def valid_email(addr):
        rfc822_specials = '/()<>@,;:\\"[]'
        # First we validate the name portion (name@domain)
        c = 0
        while c < len(addr):
                if addr[c] == '"' and (not c or addr[c - 1] == '.' or addr[c - 1] == '"'):
                        c = c + 1
                        while c < len(addr):
                                if addr[c] == '"': break
                                if addr[c] == '\\' and addr[c + 1] == ' ':
                                        c = c + 2
                                        continue
                                if ord(addr[c]) < 32 or ord(addr[c]) >= 127: return 0
                                c = c + 1
                        else: return 0
                        if addr[c] == '@': break
                        if addr[c] != '.': return 0
                        c = c + 1
                        continue
                if addr[c] == '@': break
                if ord(addr[c]) <= 32 or ord(addr[c]) >= 127: return 0
                if addr[c] in rfc822_specials: return 0
                c = c + 1
        if not c or addr[c - 1] == '.': return 0

        # Next we validate the domain portion (name@domain)
        domain = c = c + 1
        if domain >= len(addr): return 0
        count = 0
        while c < len(addr):
                if addr[c] == '.':
                        if c == domain or addr[c - 1] == '.': return 0
                        count = count + 1
                if ord(addr[c]) <= 32 or ord(addr[c]) >= 127: return 0
                if addr[c] in rfc822_specials: return 0
                c = c + 1

        return count >= 1

# return the replyto or from address in the header
def get_sender(header):
        replyto = header.get("Reply-To")
        if replyto is None:
                replyto = header.get("From")
                if replyto is None: return None
        return parseaddr(replyto)[1]

# return first available filename basename,[0-9]+
def available_file(dirname, basename):
        ver = 0
        maxdate = 0
        filename = "%s/%s,%s,%d" % (dirname, basename, hostname, ver)
        while exists(filename):
                maxdate = max(stat(filename)[ST_MTIME], maxdate)
                ver = ver + 1
                filename = "%s/%s,%s,%d" % (dirname, basename, hostname, ver)
        if ver >= maxfiles:
                return None, None
        return maxdate, filename

def formatpar(string, l=76, indent=2):
        words = split(string)
        res = ""
        ll = 0
        first = 1

        for word in words:
                if first == 1:
                        res = word
                        first = 0
                        ll = len(word)
                else:
                        if ll + len(word) > l:
                                res = res + "\n"+" "*indent+word
                                ll = len(word) + indent
                        else:
                                res = res+" "+word
                                ll = ll + len(word) + 1

        return res+"\n"

def store_message(message, filename):
        outfile = open(filename, "w")
        outfile.write(message.as_string())
        outfile.close()
        return

def write_part(outfile, part):
    charset = part.get_content_charset()
    payload = part.get_payload(decode=True)
    
    if charset is not None:
        try:
            msg = payload.decode(charset, "ignore")
        except:
            msg = payload
            charset = None
    else:
        msg = payload

    if charset is None:
        outfile.write(msg)
    else:
        try:
            outfile.write(msg.encode("latin-1", "ignore"))
        except:
            return False

    outfile.write("\n");
    return True

def copy_orders(message, filename, sender):
        # print the header first
        if writeheaders:
                from os.path import split
                dirname, basename = split(filename)
                dirname = dirname + '/headers'
                if not exists(dirname): mkdir(dirname)
                outfile = open(dirname + '/' + basename, "w")
                for name, value in message.items():
                        outfile.write(name + ": " + value + "\n")
                outfile.close()
        
        found = False
        outfile = open(filename, "w")
        if message.is_multipart():
            for part in message.get_payload():
                content_type = part.get_content_type()
                syslog("debug: found content type %s for %s" % (content_type, sender))
                if content_type=="text/plain":
                    if write_part(outfile, part):
                        found = True
                    else:
                        charset = part.get_content_charset()
                        syslog("error: could not write text/plain part (charset=%s) for %s" % (charset, sender))
                        
        else:
            if write_part(outfile, message):
                found = True
            else:
                charset = message.get_content_charset()
                syslog("error: could not write text/plain message (charset=%s) for %s" % (charset, sender))
        outfile.close()
        return found

# create a file, containing:
# game=0 locale=de file=/path/to/filename email=rcpt@domain.to
def accept(game, locale, stream):
        global rootdir, orderbase
        splitted = game.split("-")
        if len(splitted) == 2:
                game, extend = splitted
                orderbase = orderbase + ".pre-" + extend
        gamedir = rootdir+"/"+game+"/game"
        savedir = gamedir+"/"+orderbase
        # check if it's one of the pre-sent orders.
        # create the save-directories if they don't exist
        if not exists(gamedir): mkdir(gamedir)
        if not exists(savedir): mkdir(savedir)
        # parse message
        message = Parser().parse(stream)
        sender = get_sender(message)
        # write syslog
        if sender is None or valid_email(sender)==0:
                syslog("invalid email address: " + str(sender))
                return -1
        syslog("received orders from " + sender)
        # get an available filename
        lock_file(gamedir + "/orders.queue")
        maxdate, filename = available_file(savedir, prefix + sender)
        if filename is None:
                syslog("more than " + str(maxfiles) + " orders from " + sender)
                return -1
        # copy the orders to the file
        text_ok = copy_orders(message, filename, sender)
        unlock_file(gamedir + "/orders.queue")

        warning, msg, fail = None, "", False
        maildate = message.get("Date")
        if maildate != None:
            maildate = mktime_tz(parsedate_tz(maildate))
            utime(filename, (maildate, maildate))
            if maildate < maxdate:
                syslog("warning - inconsistent message date " + sender)
                warning = " (" + messages["warning-" + locale] + ")"
                msg = msg + formatpar(messages["maildate-" + locale] % (ctime(maxdate),ctime(maildate)), 76, 2) + "\n"
        else:
            syslog("warning - missing message date " + sender)
            warning = " (" + messages["warning-" + locale] + ")"
            msg = msg + formatpar(messages["nodate-" + locale], 76, 2) + "\n"
            
        if not text_ok:
                warning = " (" + messages["error-" + locale] + ")"
                msg = msg + formatpar(messages["multipart-" + locale], 76, 2) + "\n"
                syslog("rejected - no text/plain in orders from " + sender)
                unlink(filename)
                savedir = savedir + "/rejected"
                if not exists(savedir): mkdir(savedir)
                lock_file(gamedir + "/orders.queue")
                maxdate, filename = available_file(savedir, prefix + sender)
                store_message(message, filename)
                unlock_file(gamedir + "/orders.queue")
                fail = True

        if sendmail and warning is not None:
                frommail = emails["eressea"]
                if emails.has_key(game):
                    frommail = emails[game]
                subject = upper(game[0]) + game[1:] + " " + messages["subject-"+locale] + warning
                mail = "Subject: %s\nFrom: %s\nTo: %s\n\n" % (subject, frommail, sender) + msg
                from smtplib import SMTP
                server = SMTP("localhost")
                server.sendmail(frommail, sender, mail)
                server.close()
                
        if not sendmail:
                print text_ok, fail, sender
                print filename

        if not fail:
                lock_file(gamedir + "/orders.queue")
                queue = open(gamedir + "/orders.queue", "a")
                queue.write("email=%s file=%s locale=%s game=%s\n" % (sender, filename, locale, game))
                queue.close()
                unlock_file(gamedir + "/orders.queue")
                
        syslog("done - accepted orders from " + sender)

        return 0

# the main body of the script:
openlog("eressea")
game = argv[1]
locale = argv[2]
infile = stdin
if len(argv)>3:
    infile = open(argv[3], "r")
retval = accept(game, locale, infile)
if infile!=stdin:
    infile.close()
closelog()
exit(retval)

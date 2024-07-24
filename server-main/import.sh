#!/bin/sh
set -e

# Disable SSL
sed -i 's/ssl = yes/ssl = no/' /etc/dovecot/dovecot.conf
sed -i 's/ssl_cert = /#ssl_cert = /' /etc/dovecot/dovecot.conf
sed -i 's/ssl_key = /#ssl_key = /' /etc/dovecot/dovecot.conf

dovecot -F &
p=$!

# Wait for socket listen
sleep 0.05
for _ in $(seq 0 30); do
    if netstat -tulpn 2>/dev/null | grep "LISTEN" | tr -s ' ' | cut -d' ' -f4 | grep -E ":143$" >/dev/null; then
        break
    fi
    sleep 0.1
done

# Visible test cases
doveadm mailbox create -u test@comp30023 Test
/usr/libexec/dovecot/dovecot-lda -d test@comp30023 -m Test -p /mail/ed512.eml
/usr/libexec/dovecot/dovecot-lda -d test@comp30023 -m Test -p /mail/mst-results.eml
/usr/libexec/dovecot/dovecot-lda -d test@comp30023 -m Test -p /mail/header-minimal.eml

doveadm mailbox create -u test.test@comp30023 Test
/usr/libexec/dovecot/dovecot-lda -d test.test@comp30023 -m Test -p /mail/mst-results.eml

doveadm mailbox create -u test@comp30023 headers
/usr/libexec/dovecot/dovecot-lda -d test@comp30023 -m headers -p /mail/header-minimal.eml
/usr/libexec/dovecot/dovecot-lda -d test@comp30023 -m headers -p /mail/header-cap.eml
/usr/libexec/dovecot/dovecot-lda -d test@comp30023 -m headers -p /mail/header-nosubject.eml
/usr/libexec/dovecot/dovecot-lda -d test@comp30023 -m headers -p /mail/header-nested.eml
/usr/libexec/dovecot/dovecot-lda -d test@comp30023 -m headers -p /mail/header-ws.eml

doveadm mailbox create -u test@comp30023 "With Space"
/usr/libexec/dovecot/dovecot-lda -d test@comp30023 -m "With Space" -p /mail/mst-results.eml

/usr/libexec/dovecot/dovecot-lda -d test@comp30023 -m INBOX -p /mail/ed512.eml

doveadm mailbox create -u test@comp30023 more
/usr/libexec/dovecot/dovecot-lda -d test@comp30023 -m more -p /mail/mst-results-tab.eml
/usr/libexec/dovecot/dovecot-lda -d test@comp30023 -m more -p /mail/nul.eml

# -----
# Make modifications between these lines
doveadm mailbox create -u submitted@comp30023 Normal
doveadm mailbox create -u submitted@comp30023 Tricky
doveadm mailbox create -u submitted@comp30023 malformed

# Make modifications between these lines
# -----

# Enable SSL
sed -i 's/ssl = no/ssl = yes/' /etc/dovecot/dovecot.conf
sed -i 's/#ssl_cert = /ssl_cert = /' /etc/dovecot/dovecot.conf
sed -i 's/#ssl_key = /ssl_key = /' /etc/dovecot/dovecot.conf

# Shut down
kill $p
wait $p

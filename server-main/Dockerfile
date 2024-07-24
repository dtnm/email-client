FROM alpine:3 AS staging
WORKDIR /staging
RUN mkdir -p /staging/etc/dovecot /staging/mail
COPY mail/ /staging/mail/
COPY mail-submitted/ /staging/mail-submitted/
COPY import.sh /staging/import.sh
COPY dovecot-passwd dovecot-acl dovecot.conf /staging/etc/dovecot/

FROM alpine:3

RUN apk update \
    && apk add --no-cache dovecot \
    && chown dovecot:dovecot /etc/dovecot \
    && adduser --uid 5000 vmail --home /var/mail \
        --shell=/sbin/nologin --disabled-password \
    && addgroup vmail vmail \
    && chown -R vmail:vmail /var/mail
COPY --from=staging /staging /
RUN sh /import.sh \
    && rm -r /mail /import.sh
ENTRYPOINT [ "dovecot", "-F" ]

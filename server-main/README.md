# project2-server
The visible IMAP server for Project 2, COMP30023 2024.\
An instance is deployed at `comp30023-2024.cloud.edu.au`.

Alternatively, you can spin up a local instance from this code.

As this is optional, no instructions or help will be provided by tutors,
apart from feedback in the form of PR review comments
(if you choose to contribute).

## Build
```sh
docker build .
```
or
```sh
docker compose build
```

## Contribute
You may contribute additional mail or mailboxes to this repository, for the
purpose of testing your solution of project 2 against more sample mail.

If you wish to contribute, post on Ed #512 to request for write access to the
repository, and create a PR.

Rules:
- The submitted mail must be appropriate and not offensive (to any demographic)
- Do not modify the account, mail or mailboxes used by visible test cases
- Use existing mailboxes where possible
- Do not delete others' submitted mail without their permission
- The mail must be successfully submitted
- Be careful not to expose sensitive information (including Reply-To lines, Unsubscribe links etc.)

## Licenses
### Image
- Alpine Linux (MIT), contents of base image under various licenses
- Dovecot (MIT AND LGPL-2.1-or-later)

### Code
- Code contributed by teaching staff is (c) University of Melbourne 2024

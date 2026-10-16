# taskmaster

Small clone of supervisor, keep track of processes and write rules for them.

## Build

```sh
make
```

## Usage

In one terminal

```bash
./taskmasterd myconfigfile.conf
```

In the other

```bash
./taskmasterctl myconfigfile.conf
```

then type `help` for commands

## Check memory usage

```bash
valgrind --leak-check=full --show-leak-kinds=all --suppressions=readline.supp --child-silent-after-fork=yes ./taskmasterd myconfigfile.conf
```

Change to ctl for client.

check SysLog

```
journalctl -t taskmaster-prod
```

## How it works

Reads the config file then keep tracks of processes and their configs.

## Author

tfreydie & bpoyet

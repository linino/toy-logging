# Toy implementation of small logging/tracing system. #

## Building under Linux ##

   make

## Running under Linux ##

	./toy-logging

Two loops in a couple of processes are started. Each loop traces two
events (test1 and test2) every second in an alternate fashion
(test1 -> 1s -> test2 -> 1s ->test1 ....). Tracing is initially disabled for
both loops. To enable it, send a SIGUSR1 to each of the two processes.
So:

	ciminaghi@zarro:/home/develop/zephyr/toy-logging$ ./toy-logging

In another terminal:

	ciminaghi@zarro:/home/develop/zephyr/toy-logging$ ps ax | grep toy-logging
	13275 pts/6    R+     0:04 ./toy-logging
	13276 pts/6    S+     0:00 ./toy-logging
	13277 pts/6    S+     0:00 ./toy-logging
	13281 pts/9    S+     0:00 grep toy-logging
	ciminaghi@zarro:/home/develop/zephyr/toy-logging$ kill -USR1 13276

And:

	ciminaghi@zarro:/home/develop/zephyr/toy-logging$ ./toy-logging
	sigusr1_handler !
	EVENT test1 -> toy1, 32
	EVENT test2 -> toy1
	EVENT test1 -> toy1, 34
	EVENT test2 -> toy1
	EVENT test1 -> toy1, 36

To make the other process trace, just send it a SIGUSR1 (kill -USR1 13277).


## Architecture ##


To setup a trace event named `evt`, a user must include "tracing.h" and declare:

* A data structure named struct `evt_args` containing the event's payload
  (see `struct test1_args` in main.c)
* A callback named `evt_cb` receiving a pointer to a `struct traced_event`
  (see tracing.h for its definition) and a void pointer which is actually
  a pointer to a struct `evt_args`. Note that the latter pointer could be
  unaligned. It is therefore safer copying it to a local `struct evt_args`
* A function called `evt_copy_args()` receiving a pointer to a `struct evt_args`
  and the event's arguments. This function must copy such arguments into
  the structure pointed by its first parameter. Note that such pointer can
  also be unaligned, so you should initialize a local structure and copy it
  via `memcpy()`.

Finally, the declare_trace_evt() can be called as follows:

	declare_trace_evt(evt, PROTO(<prototype_of_evt_cb>), <evt_cb_arguments>)

To trigger the actual tracing of an event, call

	evt_trace(<evt_args>)


See main.c for a couple of examples.


Have fun
Davide

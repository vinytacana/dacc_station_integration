#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/epoll.h>

#define MAX_EPOLL_EVENTS 16
#define MAX_OUTPUT_PIPES 4

char Message[] = "Hello there!!#?";

void fill_pipe(int fd) {
	for (int i = 0; i < 4096; i++) {
		write(fd, Message, sizeof(Message));
	}
}

void drain_pipe(int fd) {
	char drainBuffer[4096];

	for (int i = 0; i < 4096; i++) {
		read(fd, drainBuffer, sizeof(Message));
	}
}

int main() {
	int epollFd = -1;
	int eventsReady = 0;
	int inf[MAX_OUTPUT_PIPES][2];

	struct epoll_event einf[MAX_OUTPUT_PIPES];
	struct epoll_event eventQueue[MAX_EPOLL_EVENTS];

	epollFd = epoll_create1(0);
	if (epollFd == -1) {
		printf("Couldn't create epoll instance!\n");
	}

	printf("Watching for type %d events.\n", EPOLLOUT);

	for (int i = 0; i < MAX_OUTPUT_PIPES; i++) {

		pipe(inf[i]);

		printf("fd for %d = %d, %d\n", i, inf[i][0], inf[i][1]);

		einf[i].events = EPOLLOUT;
		einf[i].data.fd = inf[i][1];

		if (epoll_ctl(epollFd, EPOLL_CTL_ADD, inf[i][1], &einf[i]) == -1) {
			printf("Couldn't add to epoll!\n");
		}

		fill_pipe(inf[i][1]);

		if (i % 2 == 0) {
			// drain_pipe(inf[i][0]);
		}
	}

	while (true) {
		epoll_ctl(epollFd, EPOLL_CTL_DEL, inf[2][1], &einf[2]);

		eventsReady = epoll_wait(epollFd, eventQueue, MAX_EPOLL_EVENTS, -1);

		for (int i = 0; i < eventsReady; i++) {
			printf("Got an event %d for fd %d!\n",
				eventQueue[i].events, eventQueue[i].data.fd);

			fill_pipe(eventQueue[i].data.fd);

			printf("Wrote the message: %s\n", Message);
		}
	}
}
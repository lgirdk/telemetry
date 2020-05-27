#include <stdio.h>
#include <telemetry_busmessage_sender.h>

int main(int argc, char *argv[]) {
    char telemetry_data[1024] = {0};
	t2_event_s(argv[1], argv[2]);

	return 0;
}

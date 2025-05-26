#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>

#define SIG_EMAIL SIGUSR1
#define SIG_REMINDER SIGUSR2
#define SIG_DOORBELL SIGINT

volatile sig_atomic_t email_received = 0;
volatile sig_atomic_t reminder_received = 0;
volatile sig_atomic_t doorbell_received = 0;

// To handle each signal.
void handle_signal(int signum) {
    if (signum == SIG_EMAIL)
        email_received = 1;
    else if (signum == SIG_REMINDER)
        reminder_received = 1;
    else if (signum == SIG_DOORBELL)
        doorbell_received = 1;
}

void setup_signal_handlers() {
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIG_EMAIL, &sa, NULL);
    sigaction(SIG_REMINDER, &sa, NULL);
    sigaction(SIG_DOORBELL, &sa, NULL);
}

void block_distractions(sigset_t *old_mask) {
    sigset_t block_set;
    sigemptyset(&block_set);
    sigaddset(&block_set, SIG_EMAIL);
    sigaddset(&block_set, SIG_REMINDER);
    sigaddset(&block_set, SIG_DOORBELL);
    sigprocmask(SIG_BLOCK, &block_set, old_mask);
}

void unblock_distractions(sigset_t *old_mask) {
    sigprocmask(SIG_SETMASK, old_mask, NULL);
}

void handle_pending() {
    sigset_t pending;
    sigpending(&pending);

    if (sigismember(&pending, SIG_EMAIL) || email_received) {
        printf(" - Email notification is waiting.\n");
        printf("[Outcome:] The TA announced: Everyone get 100 on the exercise!\n");
    }

    if (sigismember(&pending, SIG_REMINDER) || reminder_received) {
        printf(" - You have a reminder to pick up your delivery.\n");
        printf("[Outcome:] You picked it up just in time.\n");
    }

    if (sigismember(&pending, SIG_DOORBELL) || doorbell_received) {
        printf(" - The doorbell is ringing.\n");
        printf("[Outcome:] Food delivery is here.\n");
    }

    email_received = reminder_received = doorbell_received = 0;
}

void runFocusMode(int numOfRounds, int duration) {
    sigset_t old_mask;
    printf("Entering Focus Mode. All distractions are blocked.\n");

    for (int i = 1; i <= numOfRounds; i++) {
        block_distractions(&old_mask);

        printf("══════════════════════════════════════════════\n");
        printf("                Focus Round %d                \n", i);
        printf("──────────────────────────────────────────────\n");

        for (int i = 0; i < duration; i++) {
            printf("Simulate a distraction:\n");
            printf("  1 = Email notification\n");
            printf("  2 = Reminder to pick up delivery\n");
            printf("  3 = Doorbell Ringing\n");
            printf("  q = Quit\n>> ");

            char input[10];
            fgets(input, sizeof(input), stdin);

            if (input[0] == '1') kill(getpid(), SIG_EMAIL);
            else if (input[0] == '2') kill(getpid(), SIG_REMINDER);
            else if (input[0] == '3') kill(getpid(), SIG_DOORBELL);
            else if (input[0] == 'q') break;
        }

        printf("──────────────────────────────────────────────\n");
        printf("        Checking pending distractions...\n");
        printf("──────────────────────────────────────────────\n");

        unblock_distractions(&old_mask);
        handle_pending();

        printf("──────────────────────────────────────────────\n");
        printf("             Back to Focus Mode.\n");
    }

    printf("══════════════════════════════════════════════\n");
    printf("Focus Mode complete. All distractions are now unblocked.\n");
}



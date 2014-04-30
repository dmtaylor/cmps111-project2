/*	CHANGED
 *  4/28/14		Updated start_lottery (David Taylor, Forrest Kerslager)
 *	4/25/14		Added lottery sched (Forrest Kerslager)
 * 
 * This file contains the scheduling policy for SCHED
 *
 * The entry points are:
 *   do_noquantum:        Called on behalf of process' that run out of quantum
 *   do_start_scheduling  Request to start scheduling a proc
 *   do_stop_scheduling   Request to stop scheduling a proc
 *   do_nice		  Request to change the nice level on a proc
 *   init_scheduling      Called from main.c to set up/prepare scheduling
 */
#include "sched.h"
#include "schedproc.h"
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <minix/com.h>
#include <machine/archtypes.h>
#include "kernel/proc.h" /* for queue constants */

PRIVATE timer_t sched_timer;
PRIVATE unsigned balance_timeout;
/* CHANGE START */
PRIVATE unsigned ticket_pool;
/* CHANGE END */

#define BALANCE_TIMEOUT	5 /* how often to balance queues in seconds */

FORWARD _PROTOTYPE( int schedule_process, (struct schedproc * rmp)	);
FORWARD _PROTOTYPE( void balance_queues, (struct timer *tp)		);

#define DEFAULT_USER_TIME_SLICE 200
/* CHANGE START */
#define DEFAULT_USER_TICKETS 20
#define DEFAULT_SYSTEM_TICKETS 40
#define MAX_TICKETS 100
#define MAX_BLOCKS 5
#define QUEUE_WIN 16
#define QUEUE_LOSE 17
/* CHANGE END */

/*===========================================================================*
 *				do_noquantum				     *
 *===========================================================================*/

PUBLIC int do_noquantum(message *m_ptr)
{
	register struct schedproc *rmp;
	int rv, proc_nr_n;

	if (sched_isokendpt(m_ptr->m_source, &proc_nr_n) != OK) {
		printf("SCHED: WARNING: got an invalid endpoint in OOQ msg %u.\n",
		m_ptr->m_source);
		return EBADEPT;
	}

	rmp = &schedproc[proc_nr_n];
	/* CHANGE START */
	if(m_ptr->SCHEDULING_ACNT_IPC_SYNC < total_block_count) {
		take_tickets(rmp, 1);
	} else {
		give_tickets(rmp, 1);

	}
	/* Replace code below with code for adjusting ticket values */
	/*
	if (rmp->priority < MIN_USER_Q) {
		rmp->priority += 1; 
	}
	*/
	/* CHANGE END */

	if ((rv = schedule_process(rmp)) != OK) {
		return rv;
	}

	/* CHANGE START */
	start_lottery();
	/* CHANGE END */

	return OK;
}

/*===========================================================================*
 *				do_stop_scheduling			     *
 *===========================================================================*/
PUBLIC int do_stop_scheduling(message *m_ptr)
{
	register struct schedproc *rmp;
	int rv, proc_nr_n;

	/* check who can send you requests */
	if (!accept_message(m_ptr))
		return EPERM;

	if (sched_isokendpt(m_ptr->SCHEDULING_ENDPOINT, &proc_nr_n) != OK) {
		printf("SCHED: WARNING: got an invalid endpoint in OOQ msg "
		"%ld\n", m_ptr->SCHEDULING_ENDPOINT);
		return EBADEPT;
	}

	rmp = &schedproc[proc_nr_n];
	rmp->flags = 0; /*&= ~IN_USE;*/
	/* CHANGE START */
	/* Remove proc's tickets from ticket pool */
	ticket_pool -= rmp->num_tickets;
	/* CHANGE END */

	return OK;
}

/*===========================================================================*
 *				do_start_scheduling			     *
 *===========================================================================*/
PUBLIC int do_start_scheduling(message *m_ptr)
{
	register struct schedproc *rmp;
	int rv, proc_nr_n, parent_nr_n, nice;
	
	/* we can handle two kinds of messages here */
	assert(m_ptr->m_type == SCHEDULING_START || 
		m_ptr->m_type == SCHEDULING_INHERIT);

	/* check who can send you requests */
	if (!accept_message(m_ptr))
		return EPERM;

	/* Resolve endpoint to proc slot. */
	if ((rv = sched_isemtyendpt(m_ptr->SCHEDULING_ENDPOINT, &proc_nr_n))
			!= OK) {
		return rv;
	}
	rmp = &schedproc[proc_nr_n];

	/* Populate process slot */
	rmp->endpoint     = m_ptr->SCHEDULING_ENDPOINT;
	rmp->parent       = m_ptr->SCHEDULING_PARENT;
	rmp->max_priority = (unsigned) m_ptr->SCHEDULING_MAXPRIO;
	if (rmp->max_priority >= NR_SCHED_QUEUES) {
		return EINVAL;
	}

	switch (m_ptr->m_type) {

	case SCHEDULING_START:
		/* We have a special case here for system processes, for which
		 * quanum and priority are set explicitly rather than inherited 
		 * from the parent */
		rmp->priority   = rmp->max_priority;
		rmp->time_slice = (unsigned) m_ptr->SCHEDULING_QUANTUM;
		/* CHANGE START */
		give_tickets(rmp, DEFAULT_SYSTEM_TICKETS);
		/* CHANGE END */
		break;
		
	case SCHEDULING_INHERIT:
		/* Inherit current priority and time slice from parent. Since there
		 * is currently only one scheduler scheduling the whole system, this
		 * value is local and we assert that the parent endpoint is valid */
		if ((rv = sched_isokendpt(m_ptr->SCHEDULING_PARENT,
				&parent_nr_n)) != OK)
			return rv;

		rmp->priority = schedproc[parent_nr_n].priority;	/* Force into queue 17? */
		rmp->time_slice = schedproc[parent_nr_n].time_slice;
		/* CHANGE START */
		give_tickets(rmp, DEFAULT_USER_TICKETS);
		/* CHANGE END */
		break;
		
	default: 
		/* not reachable */
		assert(0);
	}

	/* Take over scheduling the process. The kernel reply message populates
	 * the processes current priority and its time slice */
	if ((rv = sys_schedctl(0, rmp->endpoint, 0, 0)) != OK) {
		printf("Sched: Error taking over scheduling for %d, kernel said %d\n",
			rmp->endpoint, rv);
		return rv;
	}
	rmp->flags = IN_USE;

	/* Schedule the process, giving it some quantum */
	if ((rv = schedule_process(rmp)) != OK) {
		printf("Sched: Error while scheduling process, kernel replied %d\n",
			rv);
		return rv;
	}

	/* Mark ourselves as the new scheduler.
	 * By default, processes are scheduled by the parents scheduler. In case
	 * this scheduler would want to delegate scheduling to another
	 * scheduler, it could do so and then write the endpoint of that
	 * scheduler into SCHEDULING_SCHEDULER
	 */

	m_ptr->SCHEDULING_SCHEDULER = SCHED_PROC_NR;

	return OK;
}

/*===========================================================================*
 *				do_nice					     *
 *===========================================================================*/
PUBLIC int do_nice(message *m_ptr)
{
	struct schedproc *rmp;
	int rv;
	int proc_nr_n;
	unsigned new_q, old_q, old_max_q;

	/* check who can send you requests */
	if (!accept_message(m_ptr))
		return EPERM;

	if (sched_isokendpt(m_ptr->SCHEDULING_ENDPOINT, &proc_nr_n) != OK) {
		printf("SCHED: WARNING: got an invalid endpoint in OOQ msg "
		"%ld\n", m_ptr->SCHEDULING_ENDPOINT);
		return EBADEPT;
	}

	rmp = &schedproc[proc_nr_n];
	new_q = (unsigned) m_ptr->SCHEDULING_MAXPRIO;
	if (new_q >= NR_SCHED_QUEUES) {
		return EINVAL;
	}

	/* Store old values, in case we need to roll back the changes */
	old_q     = rmp->priority;
	old_max_q = rmp->max_priority;

	/* Update the proc entry and reschedule the process */
	rmp->max_priority = rmp->priority = new_q;

	if ((rv = schedule_process(rmp)) != OK) {
		/* Something went wrong when rescheduling the process, roll
		 * back the changes to proc struct */
		rmp->priority     = old_q;
		rmp->max_priority = old_max_q;
	}

	return rv;
}

/*===========================================================================*
 *				schedule_process			     *
 *===========================================================================*/
PRIVATE int schedule_process(struct schedproc * rmp)
{
	int rv;

	if ((rv = sys_schedule(rmp->endpoint, rmp->priority,
			rmp->time_slice)) != OK) {
		printf("SCHED: An error occurred when trying to schedule %d: %d\n",
		rmp->endpoint, rv);
	}

	return rv;
}


/*===========================================================================*
 *				start_scheduling			     *
 *===========================================================================*/

PUBLIC void init_scheduling(void)
{
	balance_timeout = BALANCE_TIMEOUT * sys_hz();
	init_timer(&sched_timer);
	set_timer(&sched_timer, balance_timeout, balance_queues, 0);
}

/*===========================================================================*
 *				balance_queues				     *
 *===========================================================================*/

/* This function in called every 100 ticks to rebalance the queues. The current
 * scheduler bumps processes down one priority when ever they run out of
 * quantum. This function will find all proccesses that have been bumped down,
 * and pulls them back up. This default policy will soon be changed.
 */
PRIVATE void balance_queues(struct timer *tp)
{
	struct schedproc *rmp;
	int proc_nr;
	int rv;

	/* CHANGE START */
	/* Replace code below with code to balance */
	/*
	for (proc_nr=0, rmp=schedproc; proc_nr < NR_PROCS; proc_nr++, rmp++) {
		if (rmp->flags & IN_USE) {
			if (rmp->priority > rmp->max_priority) {
				rmp->priority -= 1; 
				schedule_process(rmp);
			}
		}
	}
	/* CHANGE END */

	set_timer(&sched_timer, balance_timeout, balance_queues, 0);
}

/* CHANGE START */
/*===========================================================================*
 *				give_tickets				     *
 *===========================================================================*/
PRIVATE int give_tickets(struct schedproc * rmp, int new_tickets)
{
	/* Assert max ticket amount */
	if ((rmp->num_tickets + new_tickets) <= MAX_TICKETS) {
		/* Add tickets for the process to the ticket pool */
		ticket_pool += new_tickets;
		/* Give the process the default amount of tickets */
		rmp->num_tickets += new_tickets;
	} else {
		/* Return error, no process can have more than MAX_TICKETS */
		return 1;
	}

	return 0;
}

/*===========================================================================*
 *				take_tickets				     *
 *===========================================================================*/
PRIVATE int take_tickets(struct schedproc * rmp, int old_tickets)
{
	/* Assert min ticket amount */
	if (rmp->num_tickets - old_tickets >=  1)
		/* Remove tickets from the ticket pool */
		ticket_pool -= old_tickets;
		/* Remove tickets from the process */
		rmp->num_tickets -= old_tickets;
	else {
		/* Return error, no process can have <1 tickets */
		return 1;
	}

	return 0;
}

/*===========================================================================*
 *				start_lottery				     *
 *===========================================================================*/
PRIVATE int start_lottery()
{
	int i, rsum = 0, winning_num;
    char flag_won = 0;
	struct schedproc *rmp;

	/* Generate random winning ticket */
	srandom(time(NULL));
	winning_num = random() % (ticket_pool - 1);

	/* Loop through process table, scheduling winners and losers */
	for (i = 0; i < NR_PROCS; i++){
		rmp = schedproc[i];
		rsum += rmp->num_tickets;
		
		/* Winner is found when the running sum exceeds the random number */
		if (rsum >= winning_num && !flag_won) {
			/* This process wins, set priority 16 */
            rmp->priority = QUEUE_WIN;
			/* Winner is already found, but continue setting losers */
            flag_won = 1;
		} else {
			/* This process loses, set priority to 17 */
			/* What if proc was previously a winner? should it still be in win queue? -FK */
            rmp->priority = QUEUE_LOSE;
		}

		/* Every loop iteration schedules a process as a winner or loser */
		schedule_process(rmp);
	}
}
/* CHANGE END */







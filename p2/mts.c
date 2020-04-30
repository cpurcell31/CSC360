
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>


//Defining Trains and Queues
typedef struct train {

	pthread_cond_t sigT;
	char dir;
	int loadT;
	int arrivT;
	int id;
	int status;
	struct train *next;

} Train;

//First-in-first-out Queues
typedef struct fifo {

	Train *head;
	Train *tail;
	int length;
} fifo;

Train* createTrain();
Train* removeTrain(fifo *queue);
fifo* createQueue();
fifo* addTrain(fifo *queue, Train *new);
Train* peekQueue(fifo *queue);

//Creates and returns empty train 
Train* createTrain() {
	Train *new = (Train*)malloc(sizeof(Train));
	new->next = NULL;
	return new;
}

//Creates and returns empty queue
fifo* createQueue() {
	fifo *new = (fifo*)malloc(sizeof(fifo));
	new->head = NULL;
	new->tail = NULL;
	new->length = 0;
	return new;
}

//Adds given train to given queue and returns the queue
fifo* addTrain(fifo *queue, Train *new) {
	Train *tail;
	Train *temp;
	//Empty Queue
	if(queue->head == NULL) {
		queue->head = new;
		queue->tail = new;
		queue->length++;
		return queue;
	}
	//Non-empty Queue
	//Place in queue based on id and load Time
	//Rule 4a
	if((queue->head->id > new->id) && (queue->head->loadT == new->loadT)) {
			//Shuffle
			temp = queue->head;
			queue->head = new;
			new->next = temp;
			queue->length++;
			return queue;
	}
	else {
		tail = queue->tail;
		tail->next = new;
		queue->tail = new;
		queue->length++;
		return queue;
	}
}

//Removes given train from the queue and returns it
Train* removeTrain(fifo *queue) {
	Train *head = queue->head;
	Train *temp;
	//Queue has 2 or more items
	if(queue->length == 0) {
		return NULL;
	}
	if(head->next != NULL) {
		temp = head->next;
		queue->head = temp;
	}
	//Queue has 1 item
	else {
		queue->head = NULL;
	}
	queue->length--;
	//Make sure to test for null after return
	return head;	
}

Train *peekQueue(fifo *queue) {
	Train *head = queue->head;
	return head;
}


//Globals
fifo *queueHiW;
fifo *queueLoW;
fifo *queueHiE;
fifo *queueLoE;
struct timespec timeSt;
//Mutex and Convars
pthread_mutex_t track = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t HiW = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t LoW = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t HiE = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t LoE = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t start = PTHREAD_COND_INITIALIZER;
pthread_cond_t dispatch = PTHREAD_COND_INITIALIZER;
pthread_cond_t off = PTHREAD_COND_INITIALIZER;
//pseudo convar
int offf = 0;
#define BILLION 1000000000.0;

//Prototypes
void printTrains(fifo *queue);
void* controllerTh(void *ptr);
void* trainTh(void *train);
int testRule4a(void);
int testRule4b(char lastDir);


//Initialize the threads and wait for their completion
int main(int argc, char *argv[]) {

	if(argc != 2) {
		printf("Invalid Number of Provided Arguments for p2\n");
		exit(1);
	}
	
	int checker;
	int counter = 0;
	int numLines = 0;
	int r;
	char *filename = argv[1];
	FILE *scan;
	char direction[1024];
	char lTime[1024][1024];
	char aTime[1024][1024];
	int pos;
	Train *t;
	pthread_t cthread;
	pthread_t tthreads[1024];
	queueHiW = createQueue();
	queueLoW = createQueue();
	queueHiE = createQueue();
	queueLoE = createQueue();	

	//Check to see if fopen worked
	scan = fopen(filename, "r");
	if(scan == NULL) {
		printf("Error opening file with name: %s\n", filename);
		exit(-1);
	}

	r = fscanf(scan, "%c %s %s%d", &direction[numLines], lTime[numLines], aTime[numLines], &pos);
	numLines++;
	while(r != EOF) {
		//Check to see if fscanf worked
		r = fscanf(scan, "%c %s %s%d", &direction[numLines], lTime[numLines], aTime[numLines], &pos);
		numLines++;
	}
	//Adjust range to be length of file 
	//Make this so but make train threads, trains add themselves to queue
	
	fclose(scan);	
	
	if(pthread_mutex_init(&track, NULL)) {
		printf("Error initializing mutex\n");
		exit(-1);
	}
	//init controller
	checker = pthread_create(&cthread, NULL, controllerTh, NULL);
	if(checker) {
		printf("Error in controller creation\n");
		exit(-1);
	}
	//init trains
	while(counter < numLines-1) {
		t = createTrain();
		t->dir = direction[counter];
		t->loadT = atoi(lTime[counter]);
		t->arrivT = atoi(aTime[counter]);
		t->id = counter;
		if(checker = pthread_create(&tthreads[counter], NULL, trainTh, t)) {
			printf("Error in train creation. Train %d\n", counter);
		}
		counter++;
	}

	sleep(1);
	if(clock_gettime(CLOCK_REALTIME, &timeSt) == -1) {
		printf("Error getting clock time\n");
		exit(-1);
	}
	pthread_cond_broadcast(&start);
	counter = 0;
	while(counter < numLines-1) {
		pthread_join(tthreads[counter], NULL);
		counter++;
	}

	//Cleanup
	free(queueHiW);
	free(queueLoW);
	free(queueHiE);
	free(queueLoE);
	pthread_mutex_destroy(&track);
	pthread_mutex_destroy(&HiW);
	pthread_mutex_destroy(&HiE);
	pthread_mutex_destroy(&LoE);
	pthread_mutex_destroy(&LoW);
}

//Controls all the trains and signals whose turn it is
void* controllerTh(void *ptr) {

	pthread_mutex_lock(&track);
	pthread_cond_wait(&dispatch, &track);
	pthread_mutex_unlock(&track);

	int counterW = 0;
	int counterE = 0;
	char lastDir = 'w';
	int samePrio = 0;
	int check = 0;
	Train *t;
	while(1) {
		while(!(queueHiE->length) && !(queueHiW->length) && !(queueLoE->length) && !(queueLoW->length)) {
			//pthread_cond_wait(&dispatch, &track);
		}
		if((queueHiE->length && queueHiW->length) || (queueLoE->length && queueLoW->length)) {
			samePrio = 1;
		}	
		else {
			samePrio = 0;
		}
			
		if(counterW >= 3 && (queueHiE->length || queueLoE->length)) {
			if(queueHiE->length) {
				pthread_mutex_lock(&HiE);
				t = removeTrain(queueHiE);
				pthread_mutex_unlock(&HiE);
				lastDir = 'e';
				counterE++;
				counterW = 0;	
			}
			else if(queueLoE->length) {
				pthread_mutex_lock(&LoE);
				t = removeTrain(queueLoE);
				pthread_mutex_unlock(&LoE);
				lastDir = 'e';
				counterE++;
				counterW = 0;
			}	
		}
		else if(counterE >= 3 && (queueHiW->length || queueLoW->length)) {
			if(queueHiW->length) {
				pthread_mutex_lock(&HiW);
				t = removeTrain(queueHiW);
				pthread_mutex_unlock(&HiW);
				lastDir = 'w';
				counterW++;
				counterE = 0;
			}
			else if(queueLoW->length) {
				pthread_mutex_lock(&LoW);
				t = removeTrain(queueLoW);
				pthread_mutex_unlock(&LoW);
				lastDir = 'w';
				counterW++;
				counterE = 0;
			}
		}
		else if(samePrio) {
			check = testRule4b(lastDir);
			//HiW goes
			if(check == 1) {
				pthread_mutex_lock(&HiW);
				t = removeTrain(queueHiW);
				pthread_mutex_unlock(&HiW);
				lastDir = 'w';
				counterW++;
				counterE = 0;
			}
			//HiE goes
			else if(check == 2) {
				pthread_mutex_lock(&HiE);
				t = removeTrain(queueHiE);
				pthread_mutex_unlock(&HiE);
				lastDir = 'e';
				counterE++;
				counterW = 0;
			}
			//LoW goes
			else if(check == 3) {
				pthread_mutex_lock(&LoW);
				t = removeTrain(queueLoW);
				pthread_mutex_unlock(&LoW);
				lastDir = 'w';
				counterW++;
				counterE = 0;

			}
			//LoE goes
			else if(check == 4) {
				pthread_mutex_lock(&LoE);
				t = removeTrain(queueLoE);
				pthread_mutex_unlock(&LoE);
				lastDir = 'e';
				counterE++;
				counterW = 0;

			}	
		}
		else if(queueHiE->length) {
			pthread_mutex_lock(&HiE);
			t = removeTrain(queueHiE);
			pthread_mutex_unlock(&HiE);
			lastDir = 'e';
			counterE++;
			counterW = 0;

		}
		else if(queueHiW->length) {
			pthread_mutex_lock(&HiW);
			t = removeTrain(queueHiW);
			pthread_mutex_unlock(&HiW);
			lastDir = 'w';
			counterW++;
			counterE = 0;

		}
		else if(queueLoE->length) {
			pthread_mutex_lock(&LoE);
			t = removeTrain(queueLoE);
			pthread_mutex_unlock(&LoE);
			lastDir = 'e';
			counterE++;
			counterW = 0;

		}
		else if(queueLoW->length) {
			pthread_mutex_lock(&LoW);
			t = removeTrain(queueLoW);
			pthread_mutex_unlock(&LoW);
			lastDir = 'w';
			counterW++;
			counterE = 0;

		}
		//test to see if train is ready to recieve signal
		while(t->status != 1) {
			;
		}
		pthread_mutex_lock(&track);
		pthread_cond_signal(&(t->sigT));
		pthread_mutex_unlock(&track);

		pthread_mutex_lock(&track);
		offf = 1;
		pthread_cond_wait(&off, &track);
		offf = 0;
		pthread_mutex_unlock(&track);
	}
}

//the common function for all trains 
void* trainTh(void *train) {

	Train *self = (Train*) train;
	pthread_cond_init(&(self->sigT), NULL);
	double timer;
	char *direction;
	int direct;
	int priority;
	struct timespec stop;
	double accum;
	int mins;
	int hours;
	self->status = 0;

	if(self->dir == 'e' || self->dir == 'E') {
		direction = "East";
		direct = 0;
		if(self->dir == 'e') {
			priority = 0;	
		}
		else {
			priority = 1;
		}
	}
	else {
		direction = "West";
		direct = 1;
		if(self->dir == 'w') {
			priority = 0;
		}
		else {
			priority = 1;
		}
	}

	//Does this really make all trains start at the same time?
	pthread_mutex_lock(&track);
	pthread_cond_wait(&start, &track);
	pthread_mutex_unlock(&track);

	//Somethings not quite right here?
	usleep(self->loadT*100000);

	//Print finish loading time
	if(clock_gettime(CLOCK_REALTIME, &stop) == -1) {
		printf("Error getting clock time\n");
		exit(-1);
	}
	accum = (stop.tv_sec - timeSt.tv_sec)
	       	+ (stop.tv_nsec - timeSt.tv_nsec)/BILLION;
	mins = accum / 60;
	hours = mins / 60;	
	//Fix time display to be like specifications
	printf("%02d:%02d:%04.1f Train %d is ready to go %s\n", hours, mins, accum, self->id, direction);

	//If train has high priority and travels West
	if(priority && direct) {
		pthread_mutex_lock(&HiW);
		addTrain(queueHiW, self);
		pthread_mutex_unlock(&HiW);
	}
	//If train has high priority and travels East
	else if(priority && !direct) {
		pthread_mutex_lock(&HiE);
		addTrain(queueHiE, self);
		pthread_mutex_unlock(&HiE);
	}
	//If train has low priority and travels West
	else if(!priority && direct) {
		pthread_mutex_lock(&LoW);
		addTrain(queueLoW, self);
		pthread_mutex_unlock(&LoW);
	}
	//If train has low priority and travels East
	else {
		pthread_mutex_lock(&LoE);
		addTrain(queueLoE, self);
		pthread_mutex_unlock(&LoE);
	}

	//Deadlocks at this wait
	pthread_mutex_lock(&track);
	pthread_cond_signal(&dispatch);
	self->status = 1;
	pthread_cond_wait(&(self->sigT), &track);

	//Print on track time
	if(clock_gettime(CLOCK_REALTIME, &stop) == -1) {
		printf("Error getting clock time\n");
		exit(-1);
	}
	accum = (stop.tv_sec - timeSt.tv_sec)
	       	+ (stop.tv_nsec - timeSt.tv_nsec)/BILLION;
	mins = accum / 60;
	hours = mins / 60;
	printf("%02d:%02d:%04.1f Train %d is ON the main track going %s\n", hours, mins, accum,self->id, direction);
	
	//Cross the main track
	usleep(self->arrivT*100000);

	//Print train finish time
	if(clock_gettime(CLOCK_REALTIME, &stop) == -1) {
		printf("Error getting clock time\n");
		exit(-1);
	}
	accum = (stop.tv_sec - timeSt.tv_sec)
	       	+ (stop.tv_nsec - timeSt.tv_nsec)/BILLION;
	mins = accum / 60;
	hours = mins / 60;
	printf("%02d:%02d:%04.1f Train %d is OFF the main track\n", hours, mins, accum, self->id);
	
	pthread_mutex_unlock(&track);

	//make sure dispatcher is ready to recieve signal	
	while(offf != 1) {
		;
	}
	pthread_mutex_lock(&track);
	pthread_cond_signal(&off);
	pthread_mutex_unlock(&track);

	free(self);
	pthread_exit(NULL);
}

//Test function to make sure Queues are working 
void printTrains(fifo *queue) {
	Train *temp;
	for(int i = 0; i < queue->length; i++) {
		temp = removeTrain(queue);
		if(temp != NULL) {
			printf("%c %d %d %d\n", temp->dir, temp->loadT, temp->arrivT, temp->id);
			free(temp);
		}
	}

}

//Test Rule 4a and return 0, 1, 2, 3, 4
//return value of 0 indicates error
//1 = HiW will be chosen
//2 = HiE will be chosen
//3 = LoW will be chosen
//4 = LoE will be chosen
//Obsolete
int testRule4a(void) {
	if(queueHiW->length && queueHiE->length) {
		if(queueHiW->head->id < queueHiE->head->id) {
			return 1;
		}
		else if(queueHiE->head->id < queueHiW->head->id) {
			return 2;
		}
	}
	else if(queueLoW->length && queueLoE->length) {
		if(queueLoW->head->id < queueLoE->head->id) {
			return 3;
		}
		else if(queueLoE->head->id < queueLoW->head->id) {
			return 4;
		}
	}
	return 0;
}

//Tests Rule 4b and returns 
//same values as defined by
//testRule4a()
int testRule4b(char lastDir) {
	if(lastDir == 'e') {
		if(queueHiW->length) {
			return 1;
		}
		else if(queueLoW->length) {
			return 3;
		}
	}
	else if(lastDir == 'w') {
		if(queueHiE->length) {
			return 2;
		}
		else if(queueLoE->length) {
			return 4;
		}
	}
	return 0;
}


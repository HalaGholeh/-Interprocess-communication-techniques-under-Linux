#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <semaphore.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>



//define valriables
#define STRING_LENGTH 100
#define NUMBER_OF_CASHIER 5
#define ITEMS_RANGE_NUMBER 5
#define TIME_CONSTANT 2


//queues will be used to save place for customer in cashier queue
float queue1[50][3]; //initilaized wiht the number of queue
float queue2[50][3];
float queue3[50][3];
float queue4[50][3];
float queue5[50][3];


//def fuctions
int getRandomNumber(int min, int max, int cashierId);
int deleteCustomerFromQueue(int row,float (*arr)[3], float id);
void updateChildPIDs(const char *string, int add);


// Structure to hold information about a cashier
struct CashierInfo
{
    int cashierId;
    float positiveBehavior;
    int scanningSpeed;
    float incom;
};
//*************Jamila********************//

//item struct
struct Item
{
    char name[256];
    double price;
    int quantity;
};
typedef struct Item Item;

//function for reading a file
char ** readFile( char *fileName, size_t *linesCounter)
{
    //open items file for reading
    FILE * myFile = fopen(fileName, "r");
    if (myFile == NULL)
    {
        perror("Error");
        exit(1);
    }

    char buffer[100]; //buffer to store each file temorarily
    *linesCounter = 0;
    char **lines = NULL; //pointer to an array of strings

    while (fgets(buffer, sizeof(buffer), myFile) != NULL)
    {
        buffer[strcspn(buffer, "\n")] = '\0'; //replace the new line with null
        char *line = malloc(strlen(buffer)+1);
        strcpy(line, buffer);
        lines = realloc(lines, (*linesCounter + 1) * sizeof(char *)); //resize the array of lines
        if (lines == NULL)
        {
            perror("memory allocation error...");
            exit(1);
        }
        lines[(*linesCounter) ++] = line; //add the line to the array
    }
    fclose(myFile); //close the file
    return lines;
}


//handlers, counters and flags
int cashierLeaving = 0;
int customerLeaving = 0;
int incomeThresholdFlag = 0;
int myTurn=0;
void cashierLeavingThresholdHandler(int signalNum)
{
    cashierLeaving++;
}
void customerLeavingThresholdHandler(int signalNum)
{
    customerLeaving++;
}
void cashierReachesThresholdIncomeHandler(int signalNum)
{
    incomeThresholdFlag = 1;
}
void myTurnHandler(int sig)
{
    myTurn=1;
}
///*************end_Jamila**************///
int main()
{
    // Create a named pipe for communicating with graphical tool
    const char *pipePath = "/tmp/child_pipe";
    if (mkfifo(pipePath, 0666) == -1)
    {
        if (errno != EEXIST)
        {
            perror("Error making fifo");
            return 1;
        }
    }
    //another named pipe for communicating with graphical tool
    const char *fifoPath = "/tmp/num";
    if (mkfifo(fifoPath, 0666) == -1)
    {
        if (errno != EEXIST)
        {
            perror("Error making fifo");
            return 1;
        }
    }
    //establishing connection with tool before beginning
    int fd = open(fifoPath, O_WRONLY);
    if (fd == -1)
    {
        if (errno == ENXIO)
        {
            perror("Error opening named pipe for write");
            exit(EXIT_FAILURE);
        }
    }

    close(fd);

    //program launch
    //here is parent code
    //--------------------------------------------------------------------

    //save all children pids
    int children=0;
    int childrenPids[1000];
    int leaving=0;


    //attach handlers to signals
    signal(SIGUSR1, customerLeavingThresholdHandler);
    signal(SIGUSR2, cashierLeavingThresholdHandler);
    signal(SIGTERM, cashierReachesThresholdIncomeHandler);


    size_t linesCounter1;  //counting number of items in supermarket
    size_t linesCounter2;  //counting user-define values lines
    char **items = readFile("items.txt", &linesCounter1);
    char **userDefined = readFile("userDefined.txt", &linesCounter2);


    //creat shared memory for items
    key_t shmkey = ftok("shm.txt", 65); //create unique key
    int shmid = shmget(shmkey, linesCounter1 * sizeof(struct Item), IPC_CREAT | 0666); //return identifer
    if (shmid == -1)
    {
        perror("Allocation 1 faild...\n");
        exit(1);
    }
    struct Item *shm = (struct Item *) shmat(shmid, NULL, 0); //attach to the shm


    //creat shared memory for user defined
    key_t shmkey2 = ftok("shm2.txt", 65);
    int shmid2 = shmget(shmkey2, 1024, IPC_CREAT | 0666);
    if (shmid2 == -1)
    {
        perror("Allocation 2 faild...\n");
        exit(1);
    }
    char *shm2 = (char *) shmat(shmid2, NULL, 0);


    //create shared memory for cashiers
    key_t shmkey3 = ftok("shm3.txt", 65);
    int shmid3 = shmget(shmkey3, NUMBER_OF_CASHIER * sizeof(int), IPC_CREAT | 0666);
    if (shmid3 == -1)
    {
        perror("Allocation 3 faild...\n");
        exit(1);
    }
    int *shm3 = (int *) shmat(shmid, NULL, 0);

    //copy the content of items into the shm
    for (int i = 0; i<linesCounter1; i++)
    {
        sscanf(items[i], "%[^,],%lf,%d", shm[i].name, &shm[i].price, &shm[i].quantity);
    }


    //copy the content of user defined into the shm2
    size_t offset = 0;
    for (int i = 0; i<linesCounter2; i++)
    {
        strncpy(shm2 + offset, userDefined[i], STRING_LENGTH - 1);
        offset += STRING_LENGTH;
    }


    //create pipes to connect customers with cashiers
    int pip1[2];
    int pip2[2];
    int pip3[2];
    int pip4[2];
    int pip5[2];
    int pip6[2];
    //make sure that pipes are non-blocking
    if (pipe(pip1) == -1 || pipe(pip2) == -1 || pipe(pip3) == -1 || pipe(pip4) == -1 || pipe(pip5) == -1 || pipe(pip6) == -1 )
    {
        perror("PIPE");
        exit(1);
    }
    if (fcntl(pip1[0], F_SETFL, O_NONBLOCK) == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
    if (fcntl(pip2[0], F_SETFL, O_NONBLOCK) == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
    if (fcntl(pip3[0], F_SETFL, O_NONBLOCK) == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
    if (fcntl(pip4[0], F_SETFL, O_NONBLOCK) == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
    if (fcntl(pip5[0], F_SETFL, O_NONBLOCK) == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
    if (fcntl(pip6[0], F_SETFL, O_NONBLOCK) == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }


    //create semaphores for shared memory one and two
    sem_t sem;
    sem_init(&sem, 0, 1);

    sem_t sem2;
    sem_init(&sem2, 0, 1);

    //25
    sem_t sem3;
    sem_init(&sem3, 0, 1);
    //sem_wait(sem3); sem_post(sem3);


    //free allocated memory
    for (int i = 0; i<linesCounter1; i++)
    {
        free(items[i]);
    }
    free(items);

    //free allocated memory
    for (int i = 0; i<linesCounter2; i++)
    {
        free(userDefined[i]);
    }
    free(userDefined);
    FILE *file;


///////////////myPart//////////////////////////
    //initilize the first index of Q as flag

    queue1[0][0] = 1.0;
    queue2[0][0] = 2.0;
    queue3[0][0] = 3.0;
    queue4[0][0] = 4.0;
    queue5[0][0] = 5.0;


    //reading from shared memory user defined values
    int offset2 = 0;
    int maxRang = 0;
    int minRang = 0;
    char temp[30] = "";
    int threshold = 0;
    int cashierThreshold = 0;
    int customerThreshold = 0;

    for (int i = 0; i < linesCounter2; i++)
    {
        char buffer[STRING_LENGTH];
        strncpy(buffer, shm2 + offset2, STRING_LENGTH - 1);
        buffer[STRING_LENGTH - 1] = '\0';
        offset2 += STRING_LENGTH;

        if (strstr(buffer, "cashierScanTimeRange"))
        {
            sscanf(buffer, "%s %d %d", temp, &minRang, &maxRang);
        }
        else if (strstr(buffer, "incomeThreshold"))
        {
            sscanf(buffer, "%s %d", temp, &threshold);
        }
        else if (strstr(buffer, "cashierTheshold"))
        {
            sscanf(buffer, "%s %d", temp, &cashierThreshold);
        }
        else if (strstr(buffer, "customersleavingThreshold"))
        {
            sscanf(buffer, "%s %d", temp, &customerThreshold);
        }
    }

    //fork processes
    printf("Openning :>\n\n");
    int pid;
    //process to handle communicating with graphical tool
    if((pid=fork()) == -1)
    {
        printf("Fork failure... Exiting\n");
        exit(-1);
    }
    if(pid == 0)
    {
        while(1)
        {
            //sleep(0.1);
            char buff[40];
            size_t bytes = read(pip6[0], buff, sizeof(buff));
            if (bytes == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    continue;
                }
                else
                {
                    perror("read");
                    exit(EXIT_FAILURE);
                }
            }
            if(bytes == 0)
            {
                continue;
            }
            int fd = open(fifoPath, O_WRONLY);
            if (fd == -1)
            {
                if (errno == ENXIO)
                {
                    perror("Error opening named pipe for write");
                    exit(EXIT_FAILURE);
                }
            }
            write(fd,buff, sizeof(buff) );
            close(fd);
        }
    }
    childrenPids[children]=pid;
    children++;
    // Creating 5 cashiers
    for (int counter = 0; counter < 5; counter++)
    {
        if ((pid = fork()) == -1)
        {
            printf("Fork failure... Exiting\n");
            exit(-1);
        }
        if (pid == 0)
        {

            // Code executed by the child
            //closing pipes for writing in cashiers side
            close(pip1[1]);
            close(pip2[1]);
            close(pip3[1]);
            close(pip4[1]);
            close(pip5[1]);


            //saving cashier info
            struct CashierInfo cashier;
            cashier.cashierId = counter+1; // put the value of Id
            cashier.positiveBehavior = 100; // put the value of mood
            cashier.scanningSpeed = getRandomNumber(minRang, maxRang, cashier.cashierId);
            cashier.incom = 0;
            printf("Good morning cashier %d\n", cashier.cashierId);



            //each casher select his Q and his pipe
            int *p;
            float (*arr)[3];
            if (cashier.cashierId == queue1[0][0])
            {
                arr = queue1;
                p = pip1;
            }
            if (cashier.cashierId == queue2[0][0])
            {
                arr = queue2;
                p = pip2;
            }
            if (cashier.cashierId == queue3[0][0])
            {
                arr = queue3;
                p = pip3;
            }
            if (cashier.cashierId == queue4[0][0])
            {
                arr = queue4;
                p = pip4;
            }
            if (cashier.cashierId == queue5[0][0])
            {
                arr = queue5;
                p = pip5;
            }

            //counter for customers in line
            int row = 1;
            //counter for all items of all customers in line to estimate waiting time
            int totalItems=0;
            time_t start_time1;
            time(&start_time1);
            time_t starting;
            time(&starting);
            int flag=0;
            int flag2=0;
            while (cashier.positiveBehavior > 0 )
            {
                if (cashier.incom >= threshold)
                {
                    //telling parent that he has earned the threshold income then leaving
                    kill(getppid(), SIGTERM);
                    exit(0);
                }

                //Read form Pipe what customers will send
                while(1)
                {
                    char buff[40];
                    size_t bytes = read(p[0], buff, sizeof(buff));
                    if (bytes == -1)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            break;
                        }
                        else
                        {
                            perror("read");
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if (bytes == 0)
                    {
                        // No data available (writing ends of the pipes are closed)
                        break;
                    }
                    flag2=1;
                    float id, items, price;
                    sscanf(buff, "%f,%f,%f", &id, &items, &price);
                    // Successfully parsed the components

                    //if number of items sent is 0 then this means that this customer is already in line and he wants to leave
                    if(items == 0)
                    {
                        totalItems-= deleteCustomerFromQueue(row, arr, id);
                        row--;
                        continue;
                    }
                    //add customer to line
                    printf("customer %.0f enters line of cashier %d\n", id, cashier.cashierId);
                    arr[row][0] = id;     // id
                    arr[row][1] = items;  // items
                    arr[row][2] = price;  // price
                    row++;
                    totalItems+= items;
                }

                //cashier shares his line status and his scanning speed
                shm3[(cashier.cashierId-1)] = totalItems + cashier.scanningSpeed - cashier.positiveBehavior;
                //if there is somone in line serve him
                if(row > 1)
                {
                    //telling customer that it is his turn
                    if (flag ==0 || flag2 == 1)
                    {
                        flag2=0;
                        if(flag == 0)
                        {
                            kill((int)arr[1][0], SIGINT);
                            printf("scanning for customer %.0f by cashier %d -> time estimated %.2f sec\n", arr[1][0], cashier.cashierId, arr[1][1] * cashier.scanningSpeed);
                            flag=1;
                        }
                        char leaM[40];
                        sprintf(leaM, "2 %d %d %.2f %.0f", cashier.cashierId, row-1, cashier.incom, arr[1][1]);
                        write(pip6[1],leaM, sizeof(leaM));
                    }
                    time_t ending;
                    time(&ending);
                    double timeDifference = difftime(ending, starting);
                    if(timeDifference>= cashier.scanningSpeed*arr[1][1])
                    {
                        flag = 0;
                        time(&starting);
                        //sleep(arr[1][1] * cashier.scanningSpeed);
                        cashier.incom+= arr[1][2];
                        char leaM[40];
                        sprintf(leaM, "2 %d %d %.2f %.0f", cashier.cashierId, row-2, cashier.incom, arr[1][1]);
                        write(pip6[1],leaM, sizeof(leaM));
                        printf("finished scanning for %.0f with price %.2f and total income %.2f for cashier %d\n", arr[1][0], arr[1][2],cashier.incom, cashier.cashierId);
                        totalItems-=deleteCustomerFromQueue(row,arr, arr[1][0]);
                        row--;

                    }
                }
                //updating kindness of cashier
                time_t end_time1;
                time(&end_time1);
                double time_difference1 = difftime(end_time1, start_time1);
                if(time_difference1>=5)
                {
                    cashier.positiveBehavior -= (rand() % (end_time1-start_time1)) ;
                    time(&start_time1);
                }

            }
            //at this point cashier kindness is zero and he is leaving
            shm3[counter-1] = -100;

            //25
            sem_wait(&sem3);
            int fd = open(fifoPath, O_WRONLY);
            if (fd == -1)
            {
                if (errno == ENXIO)
                {
                    perror("Error opening named pipe for write");
                    exit(EXIT_FAILURE);
                }
            }
            printf("cashier %d is leaving\n", cashier.cashierId);
            char leaM[20];
            sprintf(leaM, "0 %d 0 0 0", cashier.cashierId);
            write(fd,leaM, sizeof(leaM) );
            fflush(stdout);
            close(fd);
            sem_post(&sem3);

            //25
            //25
            kill(getppid(), SIGUSR2); //telling parent
            // Code executed by the child

            exit(5);


            //25
        }
        //save pids for cashiers
        childrenPids[children]=pid;
        children++;
    }
    if((pid = fork()) == -1)
    {
        printf("fork failure ... getting out\n");
        fflush(stdout);
        exit (2);
    }
    if (pid == 0)
    {
        updateChildPIDs("hh", 1);
        exit(0);

    }

    //getting some user-defined values
    int upper,lower, waitingThreshold, custLower, custUpper;
    offset = 0;
    for(int i = 0; i<linesCounter2 ; i++)
    {
        char buffer[STRING_LENGTH];
        strncpy(buffer, shm2 + offset, STRING_LENGTH - 1);
        buffer[STRING_LENGTH - 1] = '\0';
        offset += STRING_LENGTH;
        if(i==0)
        {
            char* custRange=strtok(buffer, " ");
            custRange=strtok(NULL," ");
            custLower= atoi(custRange);
            custRange=strtok(NULL," ");
            custUpper = atoi(custRange);
        }
        if(i==1)
        {
            char* range=strtok(buffer, " ");
            range=strtok(NULL," ");
            lower= atoi(range);
            range=strtok(NULL," ");
            upper = atoi(range);

        }
        else if(i==3)
        {
            char* thresh=strtok(buffer, " ");
            thresh=strtok(NULL," ");
            waitingThreshold= atoi(thresh);  //waiting threshold for customers
        }

    }

    //seed rand function
    srand(time(NULL));
    time_t timeS,timeE;
    time(&timeS);
    int c=0;
    //setup finished let us open doors
    while(1)
    {
        //new user-defined number of customers every user-defined time
        int newCustomersNum=rand()%(custUpper-custLower +1) +custLower; //here we need to define random number of customers after each amount of time
        time(&timeE);
        double timeD= difftime(timeE, timeS);
        if(timeD>=TIME_CONSTANT || c==0)
        {
            time(&timeS);
            c=1;

            for(int counter=0; counter< newCustomersNum; counter++)
            {
                if ((pid = fork()) == -1)
                {
                    printf("fork failure ... getting out\n");
                    fflush(stdout);
                    exit (2);
                    break;
                }

                if(pid == 0)
                {
                    //here is customer code
                    //close pipes for reading
                    close(pip1[0]);
                    close(pip2[0]);
                    close(pip3[0]);
                    close(pip4[0]);
                    close(pip5[0]);

                    printf("customer %d enters supermarket\n", getpid());

                    //handler helps knowing that it is this customer turn or not yet
                    signal(SIGINT, myTurnHandler);

                    //shopping time
                    srand(time(NULL) + counter);
                    int sleepingTime = rand() % (upper - lower + 1) +lower;
                    sleep (sleepingTime);
                    //Done with shopping

                    //items to buy
                    int numItemsToBuy= (rand() % ITEMS_RANGE_NUMBER)+1;
                    Item inCartItems[numItemsToBuy];
                    int itemsIndecies[numItemsToBuy];


                    //semaphore to items shared memory to avoid conflicts
                    sem_wait(&sem);

                    //getting items in cart
                    int numItemsInCart=0;
                    int count=0;
                    double totalPrice=0;
                    for(int counter=0; counter<numItemsToBuy; counter++)
                    {
                        int itemNumber= rand() % linesCounter1;
                        int quantity= (rand() % ITEMS_RANGE_NUMBER) +1;
                        if(quantity > shm[itemNumber].quantity)
                        {
                            if(shm[itemNumber].quantity == 0)     //if quantity for this item is zero move on
                                continue;
                            quantity= shm[itemNumber].quantity;
                            shm[itemNumber].quantity-= quantity;  //update quantity for this item in shared memory
                        }
                        //get things in cart
                        strcpy(inCartItems[count].name, shm[itemNumber].name);
                        inCartItems[count].price= shm[itemNumber].price;
                        inCartItems[count].quantity= quantity;
                        numItemsInCart+= quantity;
                        itemsIndecies[count]= itemNumber;
                        totalPrice+= (inCartItems[count].price * quantity);
                        count++;
                    }
                    //release semaphore
                    sem_post(&sem);


                    //choosing cahsier by reading shared memory values of each cashier
                    int cashierPoints= shm3[0];
                    int cashierIndex=0;
                    for(int counter=1; counter<5; counter++)
                    {
                        if(shm3[counter] < cashierPoints)
                        {
                            cashierPoints= shm3[counter];
                            cashierIndex= counter;
                        }
                    }

                    //send by pipe message to the cashier you want to deal with contains pid and number of items with you and the price
                    char message[40];
                    sprintf(message, "%d,%d,%.2lf", getpid(), numItemsInCart, totalPrice);
                    switch(cashierIndex)
                    {
                    case 0:
                        sem_wait(&sem2);
                        if (write(pip1[1], message, sizeof(message)) == -1)
                        {
                            perror("write");
                            exit(EXIT_FAILURE);
                        }
                        break;
                    case 1:
                        sem_wait(&sem2);
                        if (write(pip2[1], message, sizeof(message)) == -1)
                        {
                            perror("write");
                            exit(EXIT_FAILURE);
                        }
                        break;
                    case 2:
                        sem_wait(&sem2);
                        if (write(pip3[1], message, sizeof(message)) == -1)
                        {
                            perror("write");
                            exit(EXIT_FAILURE);
                        }
                        break;
                    case 3:
                        sem_wait(&sem2);
                        if (write(pip4[1], message, sizeof(message)) == -1)
                        {
                            perror("write");
                            exit(EXIT_FAILURE);
                        }
                        break;
                    case 4:
                        sem_wait(&sem2);
                        if (write(pip5[1], message, sizeof(message)) == -1)
                        {
                            perror("write");
                            exit(EXIT_FAILURE);
                        }
                        break;
                    default:
                        break;
                    }
                    sem_post(&sem2);
                    //calculating waiting time
                    time_t start_time;
                    time(&start_time);
                    while(1)
                    {
                        time_t end_time;
                        time(&end_time);
                        double time_difference = difftime(end_time, start_time);

                        //if waiting time is more than threshold and it is not yet this customer turn then he wants to leave :<
                        if(time_difference >= waitingThreshold)
                        {
                            if(!myTurn)
                            {
                                //send message to cashier to leave his line
                                printf("customer %d is leaving from cashier %d line\n", getpid(), cashierIndex+1);
                                char exitMessage[20];
                                sprintf(exitMessage, "%d,%d,%.2lf", getpid(), 0, 0.0);
                                switch(cashierIndex)
                                {
                                case 0:
                                    sem_wait(&sem2);
                                    if (write(pip1[1], exitMessage, strlen(exitMessage)+1) == -1)
                                    {
                                        perror("write");
                                        exit(EXIT_FAILURE);
                                    }
                                    break;
                                case 1:
                                    sem_wait(&sem2);
                                    if (write(pip2[1], exitMessage, strlen(exitMessage)+1) == -1)
                                    {
                                        perror("write");
                                        exit(EXIT_FAILURE);
                                    }
                                    break;
                                case 2:
                                    sem_wait(&sem2);
                                    if (write(pip3[1], exitMessage, strlen(exitMessage)+1) == -1)
                                    {
                                        perror("write");
                                        exit(EXIT_FAILURE);
                                    }
                                    break;
                                case 3:
                                    sem_wait(&sem2);
                                    if (write(pip4[1], exitMessage, strlen(exitMessage)+1) == -1)
                                    {
                                        perror("write");
                                        exit(EXIT_FAILURE);
                                    }
                                    break;
                                case 4:
                                    sem_wait(&sem2);
                                    if (write(pip5[1], exitMessage, strlen(exitMessage)+1) == -1)
                                    {
                                        perror("write");
                                        exit(EXIT_FAILURE);
                                    }
                                    break;
                                default:
                                    break;
                                }
                                sem_post(&sem2);

                                //tell parent that you are leaving
                                kill(getppid(), SIGUSR1);

                                //return all items
                                for(int counter=0; counter<count; counter++)
                                {
                                    sem_wait(&sem);
                                    shm[itemsIndecies[counter]].quantity+= inCartItems[counter].quantity;
                                    sem_post(&sem);
                                }
                                exit(1);
                            }
                            else
                                exit(EXIT_SUCCESS);
                        }
                    }



                }

                //parent code
                //save pids for customers
                childrenPids[children]=pid;
                children++;

            }
        }
        if(customerLeaving!= leaving)
        {
            leaving = customerLeaving;
            char leaM[40];
            sprintf(leaM, "1 %d 0 0 0", leaving);
            puts(leaM);
            write(pip6[1],leaM, sizeof(leaM));
        }

        //handling closing states
        if (cashierLeaving >= cashierThreshold || customerLeaving >= customerThreshold || incomeThresholdFlag == 1)
        {
            sleep(1);
            int exitFlag=0;
            if(cashierLeaving>=cashierThreshold)
            {
                printf("too many cashiers has left. we are closing...\n");
                exitFlag=3;
            }
            else if(customerLeaving>=customerThreshold)
            {
                printf("too many customers has left. we are closing...\n");
                exitFlag=4;
            }
            else
            {
                printf("we hit the target for today come the next day :>\n");
                exitFlag=5;
            }
            printf("Kicking customers and employees out\n");

            //close ipcs
            //destroy semaphores
            sem_destroy(&sem);
            sem_destroy(&sem2);
            sem_destroy(&sem3);

            //close the pipes
            close(pip1[0]);
            close(pip1[1]);
            close(pip2[0]);
            close(pip2[1]);
            close(pip3[0]);
            close(pip3[1]);
            close(pip4[0]);
            close(pip4[1]);
            close(pip5[0]);
            close(pip5[1]);

            shmdt(shm); //detach from shm
            shmctl(shmid, IPC_RMID, NULL); //remove the shared memory

            shmdt(shm2);
            shmctl(shmid2, IPC_RMID, NULL);

            shmdt(shm3);
            shmctl(shmid3, IPC_RMID, NULL);

            //killing processes
            for(int counter=0; counter<children; counter++)
            {
                kill(childrenPids[counter], 9);

            }
            char leaM[40];
            sprintf(leaM, "%d 0 0 0 0", exitFlag);
            int fd = open(fifoPath, O_WRONLY);
            if (fd == -1)
            {
                if (errno == ENXIO)
                {
                    perror("Error opening named pipe for write");
                    exit(EXIT_FAILURE);
                }
            }
            write(fd,leaM, sizeof(leaM) );
            close(fd);
            sleep(2);
            //exit the program
            exit(EXIT_SUCCESS);
        }

        //sleep(TIME_CONSTANT);//create customers after waiting amount of time

    }

    return 0;
}



//functions part
int getRandomNumber(int min, int max, int cashierId)
{
    srand(time(NULL) + cashierId);
    int num = rand() % (max - min + 1) + min;
    return num;

}





//function to delete customer from queue
int deleteCustomerFromQueue(int size, float(*arr)[3], float id)
{
    int flag=0;
    int items=0;
    for(int counter=1; counter<size; counter++)
    {
        if(arr[counter][0] == id)
        {
            flag=1;
            items= arr[counter][1];
        }
        if(flag==1)
        {
            arr[counter-1][0]=arr[counter][0];
            arr[counter-1][1]=arr[counter][1];
            arr[counter-1][2]=arr[counter][2];
        }
    }
    return items;
}

//////////////////////////////
void updateChildPIDs(const char *string, int add)
{
    int fd;

    if (add)
    {
        fd = open("/tmp/child_pipe", O_WRONLY);
        if (fd == -1)
        {
            perror("Error opening named pipe for write");
            exit(1);
        }
        write(fd, string, strlen(string));
        close(fd);
    }
}

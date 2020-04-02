/******************************************************************** \

Control and slow readout of BRB (Big Red Board), aka mPMT mainboard

  $Id$
\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "iostream"
#include "KOsocket.h"
#include "midas.h"
#include "mfe.h"
#include "unistd.h"
#include "time.h"
#include "sys/time.h"
#include <stdint.h>



#define  EQ_NAME   "BRB"
#define  EQ_EVID   1
#define  EQ_TRGMSK 0x1111

/* Hardware */
extern HNDLE hDB;

/* make frontend functions callable from the C framework */

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
const char *frontend_name = "febrb";
/* The frontend file name, don't change it */
const char *frontend_file_name = (char*)__FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = FALSE;

/* a frontend status page is displayed with this frequency in ms */
INT display_period = 000;

/* maximum event size produced by this frontend */
INT max_event_size = 32 * 34000;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
INT max_event_size_frag = 5 * 1024 * 1024;

/* buffer size to hold events */
INT event_buffer_size = 2 * max_event_size + 10000;


/*-- Function declarations -----------------------------------------*/
INT frontend_init();
INT frontend_exit();
INT begin_of_run(INT run_number, char *error);
INT end_of_run(INT run_number, char *error);
INT pause_run(INT run_number, char *error);
INT resume_run(INT run_number, char *error);
INT frontend_loop();
extern void interrupt_routine(void);
INT read_slow_control(char *pevent, INT off);


/*-- Equipment list ------------------------------------------------*/
#undef USE_INT
EQUIPMENT equipment[] = {

  { EQ_NAME,                 /* equipment name */
    {
      EQ_EVID, EQ_TRGMSK,     /* event ID, trigger mask */
      "SYSTEM",              /* event buffer */
      EQ_PERIODIC ,      /* equipment type */
      LAM_SOURCE(0, 0x8111),     /* event source crate 0, all stations */
      "MIDAS",                /* format */
      TRUE,                   /* enabled */
      RO_ALWAYS | RO_ODB,             /* read always */
      500,                    /* poll for 500ms */
      0,                      /* stop run after this event limit */
      0,                      /* number of sub events */
      1,                      /* do log history */
      "", "", "",
    },
    read_slow_control,       /* readout routine */
  },
  {""}
};



/********************************************************************\
              Callback routines for system transitions

  These routines are called whenever a system transition like start/
  stop of a run occurs. The routines are called on the following
  occations:

  frontend_init:  When the frontend program is started. This routine
                  should initialize the hardware.

  frontend_exit:  When the frontend program is shut down. Can be used
                  to releas any locked resources like memory, commu-
                  nications ports etc.

  begin_of_run:   When a new run is started. Clear scalers, open
                  rungates, etc.

  end_of_run:     Called on a request to stop a run. Can send
                  end-of-run event and close run gates.

  pause_run:      When a run is paused. Should disable trigger events.

  resume_run:     When a run is resumed. Should enable trigger events.
\********************************************************************/

/********************************************************************/

int gAddress; // BRB address
int gPort; // BRB port


/*-- Sequencer callback info  --------------------------------------*/

/*
void seq_callback(INT hDB, INT hseq, void *info)
{
  KEY key;

  printf("odb ... Settings %x touched\n", hseq);
}

BOOL gStartHome;
BOOL gStartMove;
HNDLE handleHome;
HNDLE handleMove;


INT start_move(){

  // TOFIX: add some checks that we aren't already moving
  if(0){ // Something went wrong
    cm_msg(MERROR,"start_move","Error in move! \n");
  }

  if(!gStartMove) return 0; // Just return if move not requested...

  std::string path;
  path += "/Equipment/";
  path += EQ_NAME;
  path += "/Settings";

  // Get the destination position
  std::string destpath = path + "/Destination";
  float destination[2] = {0,0};
  int size = sizeof(destination);
  int status = db_get_value(hDB, 0, destpath.c_str(), &destination, &size, TID_FLOAT, TRUE);

  // Get the velocity
  std::string velpath = path + "/Velocity";
  float velocity[2] = {1,1};
  size = sizeof(velocity);
  status = db_get_value(hDB, 0, velpath.c_str(), &velocity, &size, TID_FLOAT, TRUE);
        
  //printf("Moving to position X=%f, Y=%f with speed %f %f\n",destination[0],destination[1],velocity[0], velocity[1]);
  cm_msg(MINFO,"start_move","Moving to position X=%f, Y=%f with speed %f %f\n",destination[0],destination[1],velocity[0], velocity[1]);

  // TOFIX: instruct the Arduino to move to the specified destination at specified speed.
  
  for(int i = 0; i < 5; i++){
    sleep(1);
    printf(".");
  }
  printf("\nFinished move\n");

  // Little magic to reset the key to 'n' without retriggering hotlink
  BOOL move = false;
  db_set_data_index1(hDB, handleMove, &move, sizeof(move), 0, TID_BOOL, FALSE);

  return 0;

}

INT start_home(){

  // TOFIX: add some checks that we aren't already moving

  if(!gStartHome) return 0; // Just return if home not requested...


  printf("Start home...\n");
  sleep(3);
  // TOFIX: instruct the Arduino to home
  printf("Finished home\n");

  // Little magic to reset the key to 'n' without retriggering hotlink
  BOOL home = false;
  db_set_data_index1(hDB, handleHome, &home, sizeof(home), 0, TID_BOOL, FALSE);

  return 0;
  }*/

KOsocket *gSocket;

/*-- Frontend Init -------------------------------------------------*/
INT frontend_init()
{

  // Setup connection to Arduino
  // TOFIX!!!
  // 


  // setup connection to ODB (online database)
  int status = cm_get_experiment_database(&hDB, NULL);
  if (status != CM_SUCCESS) {
    cm_msg(MERROR, "frontend_init", "Cannot connect to ODB, cm_get_experiment_database() returned %d", status);
    return FE_ERR_ODB;
  }

  std::string path;
  path += "/Equipment/";
  path += EQ_NAME;
  path += "/Settings";


  // Setup the socket connection

  // Get Address
  std::string varpath = path + "/address";
  gAddress = 0;
  int size = sizeof(gAddress);
  status = db_get_value(hDB, 0, varpath.c_str(), &gAddress, &size, TID_BOOL, TRUE);


  //  int gAddress; // BRB address                                                                                                                                
  //int gPort; // BRB port                                                                                                                                      
  // Open socket
  std::cout << "Opening socket... " << std::endl;
  gSocket = new KOsocket("brb00", 40);
  std::cout << "Finished;... " << std::endl;
  std::cout << "Socket status : " << gSocket->getErrorString() << std::endl;





  return SUCCESS;
}

/*-- Frontend Exit -------------------------------------------------*/
INT frontend_exit()
{

  std::cout << "Closing socket." << std::endl;
  gSocket->shutdown();
  // Close connection to Arduino
  // TOFIX!!!
  //

  return SUCCESS;
}



/*-- Begin of Run --------------------------------------------------*/
INT begin_of_run(INT run_number, char *error)
{

  //------ FINAL ACTIONS before BOR -----------
  printf("End of BOR\n");

  return SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/
INT end_of_run(INT run_number, char *error)
{

  printf("EOR\n");
  
  return SUCCESS;
}

/*-- Pause Run -----------------------------------------------------*/
INT pause_run(INT run_number, char *error)
{
  return SUCCESS;
}

/*-- Resume Run ----------------------------------------------------*/
INT resume_run(INT run_number, char *error)
{
  return SUCCESS;
}

/*-- Frontend Loop -------------------------------------------------*/
INT frontend_loop()
{

  return SUCCESS;
}

/*------------------------------------------------------------------*/
/********************************************************************\
  Readout routines for different events
\********************************************************************/
int Nloop, Ncount;

/*-- Trigger event routines ----------------------------------------*/
 INT poll_event(INT source, INT count, BOOL test)
/* Polling routine for events. Returns TRUE if event
   is available. If test equals TRUE, don't return. The test
   flag is used to time the polling */
{
  register int i;  // , mod=-1;
  register int lam = 0;

  for (i = 0; i < count; i++) {
    
    if (lam) {
      if (!test){
        return lam;
      }
    }
  }
  return 0;
}

/*-- Interrupt configuration ---------------------------------------*/
 INT interrupt_configure(INT cmd, INT source, POINTER_T adr)
{
  switch (cmd) {
  case CMD_INTERRUPT_ENABLE:
    break;
  case CMD_INTERRUPT_DISABLE:
    break;
  case CMD_INTERRUPT_ATTACH:
    break;
  case CMD_INTERRUPT_DETACH:
    break;
  }
  return SUCCESS;
}

int dummy_counter = 0;

/*-- Event readout -------------------------------------------------*/
INT read_slow_control(char *pevent, INT off)
{


  bk_init32(pevent);
  if(0){
  float *pddata;
  
  // Bank names must be exactly four char
  bk_create(pevent, "BRV0", TID_FLOAT, (void**)&pddata);

  for(int j = 0; j < 8; j++){

    double resistor = 1.0;
    if(j==0){
      std::cout << "LDO1: " ;//" << std::endl;
      resistor = 0.1;
    }
    if(j==1) std::cout << "LDO2: " ;//" << std::endl;
    if(j==2){
      std::cout << "LDO3: " ;//" << std::endl;
      resistor =200;
    }
    if(j==3){
      std::cout << "LDO4: " ;//" << std::endl;
    }
    if(j==4){
      std::cout << "LDO5: " ;//" << std::endl;
    }
    if(j==5){
      std::cout << "LDO6: " ;//" << std::endl;
      resistor =0.1;
    }
    if(j==6){
      std::cout << "reg 77: " ;//" << std::endl;                                                                                              
      resistor =0.05;
    }
    if(j==7){
      std::cout << "reg 78: " ;//" << std::endl;                                                                                              
      resistor =0.05;
    }


    struct timeval t1;  
    gettimeofday(&t1, NULL);

    // Read a current/voltage sensor
    char buffer[200];
    sprintf(buffer,"uart_read_all_ctrl %i 0\r\n",j+71);
    int size=sizeof(buffer);
    gSocket->write(buffer,size);
    
    int counter = 0;
    bool notdone = true;
    while(counter < 10000 && notdone){
      //    std::cout << "Checking counter" << counter <<  std::endl;
      if(gSocket->available()){
	notdone = false;
      }else{
	usleep(10000);
      }
      counter++;      
    }
    struct timeval t2;  
    gettimeofday(&t2, NULL);
      
    double dtime = t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec)/1000000.0;

    char bigbuffer[500];
    size = sizeof(bigbuffer);
    int val = gSocket->read(bigbuffer,size);
    std::string readback(bigbuffer);
    std::vector<std::string> values;
    std::size_t current, previous = 0;
    current = readback.find("+");
    while (current != std::string::npos) {
      values.push_back(readback.substr(previous, current - previous));
      previous = current + 1;
      current = readback.find("+", previous);
    }
    values.push_back(readback.substr(previous, current - previous));
    
    for(int i = 0; i < values.size()-1; i++){
            std::cout << values[i] << ", ";
    }
    std::cout << " |  resistance / current / voltage : ";
    for(int i = 0; i < values.size()-2; i++){
      //      std::cout << values[i] << ", ";
      if(i == 3){
	long int ivolt = strtol(values[i].c_str(),NULL,0);
	double shunt_volt = ((double)(ivolt & 0xfff )) *0.00001; 
	float shunt_current = 1000.0*shunt_volt/resistor;
	std::cout << resistor << "ohm, ";
	//std::cout << shunt_volt << "V, ";
	std::cout << shunt_current << "mA, ";
	if(j==2){
	  shunt_current = 1000.0*(shunt_volt/resistor)/0.0004;
	  std::cout << "Special LDO3 current = "  << shunt_current << "mA, ";
	}
	*pddata++ = shunt_current;
      }
      if(i == 5){
	long int ivolt = strtol(values[i].c_str(),NULL,0);
	double bus_volt = ((double)(ivolt >>3)) *0.004; 
	std::cout << bus_volt <<"V ";
	*pddata++ = bus_volt;
      }
    }
    std::cout << " dt=" << dtime <<std::endl;

  }
    

  bk_close(pevent, pddata);	
  }

  for(int i = 1; i < 4; i++){

    struct timeval t1;  
    gettimeofday(&t1, NULL);

    // Read a current/voltage sensor
    char buffer[200];
    sprintf(buffer,"custom_command get_temp %i\r\n",i);
    int size=sizeof(buffer);
    gSocket->write(buffer,size);
    
    int counter = 0;
    bool notdone = true;
    while(counter < 10000 && notdone){
      //    std::cout << "Checking counter" << counter <<  std::endl;
      if(gSocket->available()){
	notdone = false;
      }else{
	usleep(10000);
      }
      counter++;      
    }
    struct timeval t2;  
    gettimeofday(&t2, NULL);
      
    double dtime = t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec)/1000000.0;

    char bigbuffer[500];
    size = sizeof(bigbuffer);
    int val = gSocket->read(bigbuffer,size);
    std::string readback(bigbuffer);
    std::cout << i << " : " << readback << "  (dtime="<< dtime << ")" << std::endl;
    //    custom_command get_temp X 
  }

  return bk_size(pevent);

}
 


 
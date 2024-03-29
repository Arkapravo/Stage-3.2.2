#ifndef _STAGE_PLAYER_DRIVER_H
#define _STAGE_PLAYER_DRIVER_H

#include <unistd.h>
#include <string.h>
#include <math.h>

#include <libplayercore/playercore.h>

#include "../libstage/stage.hh"

#define DRIVER_ERROR(X) printf( "Stage driver error: %s\n", X )

// foward declare;
class Interface;
class StgTime;

class StgDriver : public Driver
{
 public:
  // Constructor; need that
  StgDriver(ConfigFile* cf, int section);

  // Destructor
  ~StgDriver(void);

  // Must implement the following methods.
  virtual int Setup();
  virtual int Shutdown();
  virtual int ProcessMessage(QueuePointer &resp_queue,
			     player_msghdr * hdr,
			     void * data);
  virtual int Subscribe(QueuePointer &queue, player_devaddr_t addr);
  virtual int Unsubscribe(QueuePointer &queue, player_devaddr_t addr);

  /// The server thread calls this method frequently. We use it to
  /// check for new commands and configs
  virtual void Update();

  /// all player devices share the same Stage world (for now)
  static Stg::WorldGui* world;

  /// find the device record with this Player id
  Interface* LookupDevice( player_devaddr_t addr );

  Stg::Model* LocateModel( char* basename,
									player_devaddr_t* addr,
									const std::string& type );
  
 protected:

  /// an array of pointers to Interface objects, defined below
  //GPtrArray* devices;
	std::vector<Interface*> devices;
};


class Interface
{
 public:
  Interface(player_devaddr_t addr,
            StgDriver* driver,
            ConfigFile* cf,
            int section );

  virtual ~Interface( void ){ /* TODO: clean up*/ };

  player_devaddr_t addr;
  double last_publish_time;
  double publish_interval_msec;

  StgDriver* driver; // the driver instance that created this device

  virtual int ProcessMessage(QueuePointer &resp_queue,
       			     player_msghdr_t* hdr,
			     void* data) { return(-1); } // empty implementation

  virtual void Publish( void ){}; // do nothing
  virtual void Subscribe( void ){}; // do nothing
  virtual void Unsubscribe( void ){}; // do nothing
  virtual void Subscribe( QueuePointer &queue ){}; // do nothing
  virtual void Unsubscribe( QueuePointer &queue ){}; // do nothing};
};

class InterfaceSimulation : public Interface
{
 public:
  InterfaceSimulation( player_devaddr_t addr,  StgDriver* driver,ConfigFile* cf, int section );
  virtual ~InterfaceSimulation( void ){ /* TODO: clean up*/ };
  virtual int ProcessMessage(QueuePointer & resp_queue,
                             player_msghdr_t* hdr,
                             void* data);
};

// base class for all interfaces that are associated with a model
class InterfaceModel

 : public Interface
{
 public:
  InterfaceModel( player_devaddr_t addr,
		  StgDriver* driver,
		  ConfigFile* cf,
		  int section,
		  const std::string& type );

  Stg::Model* mod;

  virtual ~InterfaceModel( void ){ /* TODO: clean up*/ };

  virtual void Subscribe( void ){ this->mod->Subscribe(); };
  virtual void Unsubscribe( void ){ this->mod->Unsubscribe(); };
};


class InterfacePosition : public InterfaceModel
{
 public:
  InterfacePosition( player_devaddr_t addr, StgDriver* driver, ConfigFile* cf, int section );
  virtual ~InterfacePosition( void ){ /* TODO: clean up*/ };
  virtual void Publish( void );
  virtual int ProcessMessage(QueuePointer & resp_queue,
                             player_msghdr_t* hdr,
                             void* data);
};

class InterfaceGripper : public InterfaceModel
{
 public:
  InterfaceGripper( player_devaddr_t addr, StgDriver* driver, ConfigFile* cf, int section );
  virtual ~InterfaceGripper( void ){ /* TODO: clean up*/ };
  virtual int ProcessMessage(QueuePointer & resp_queue,
                             player_msghdr_t* hdr,
                             void* data);
  virtual void Publish( void );
};

class InterfaceWifi : public InterfaceModel
{
 public:
  InterfaceWifi( player_devaddr_t addr, StgDriver* driver, ConfigFile* cf, int section );
  virtual ~InterfaceWifi( void ){ /* TODO: clean up*/ };
  virtual int ProcessMessage(QueuePointer & resp_queue,
                             player_msghdr_t* hdr,
                             void* data);
  virtual void Publish( void );
};

class InterfaceSpeech : public InterfaceModel
{
 public:
  InterfaceSpeech( player_devaddr_t addr, StgDriver* driver, ConfigFile* cf, int section );
  virtual ~InterfaceSpeech( void ){ /* TODO: clean up*/ };
  virtual int ProcessMessage(QueuePointer & resp_queue,
                             player_msghdr_t* hdr,
                             void* data);
  virtual void Publish( void );
};

class InterfaceLaser : public InterfaceModel
{
  private:
    int scan_id;
 public:
  InterfaceLaser( player_devaddr_t addr, StgDriver* driver, ConfigFile* cf, int section );
  virtual ~InterfaceLaser( void ){ /* TODO: clean up*/ };
  virtual int ProcessMessage(QueuePointer & resp_queue,
			      player_msghdr_t* hdr,
			      void* data);
  virtual void Publish( void );
};

/*  class InterfaceAio : public InterfaceModel */
/* { */
/*  public: */
/*   InterfaceAio( player_devaddr_t addr, StgDriver* driver, ConfigFile* cf, int section ); */
/*   virtual ~InterfaceAio( void ){ /\* TODO: clean up*\/ }; */
/*   virtual int ProcessMessage(QueuePointer & resp_queue, */
/* 			      player_msghdr_t* hdr, */
/* 			      void* data); */
/*   virtual void Publish( void ); */
/* }; */


/* class InterfaceDio : public InterfaceModel */
/* { */
/* public: */
/* 	InterfaceDio(player_devaddr_t addr, StgDriver* driver, ConfigFile* cf, int section); */
/* 	virtual ~InterfaceDio(); */
/* 	virtual int ProcessMessage(QueuePointer & resp_queue, player_msghdr_t* hdr, void* data); */
/* 	virtual void Publish(); */
/* }; */


class InterfacePower : public InterfaceModel
{
 public:
  InterfacePower( player_devaddr_t addr, StgDriver* driver, ConfigFile* cf, int section );
  virtual ~InterfacePower( void ){ /* TODO: clean up*/ };

  virtual int ProcessMessage( QueuePointer & resp_queue,
			      player_msghdr * hdr,
			      void * data );

  virtual void Publish( void );
};

class InterfaceFiducial : public InterfaceModel
{
 public:
  InterfaceFiducial( player_devaddr_t addr, StgDriver* driver, ConfigFile* cf, int section );
  virtual ~InterfaceFiducial( void ){ /* TODO: clean up*/ };

  virtual void Publish( void );
  virtual int ProcessMessage(QueuePointer & resp_queue,
                             player_msghdr_t* hdr,
                             void* data);
};


class InterfaceActArray : public InterfaceModel
{
 public:
	 InterfaceActArray( player_devaddr_t addr, StgDriver* driver, ConfigFile* cf, int section );
  virtual ~InterfaceActArray( void ){ /* TODO: clean up*/ };

  virtual int ProcessMessage( QueuePointer & resp_queue,
			      player_msghdr * hdr,
			      void * data );
  virtual void Publish( void );
};

class InterfaceBlobfinder : public InterfaceModel
{
 public:
  InterfaceBlobfinder( player_devaddr_t addr, StgDriver* driver, ConfigFile* cf, int section );
  virtual ~InterfaceBlobfinder( void ){ /* TODO: clean up*/ };

  virtual int ProcessMessage( QueuePointer & resp_queue,
			      player_msghdr * hdr,
			      void * data );
  virtual void Publish( void );
};

class InterfacePtz : public InterfaceModel
{
 public:
  InterfacePtz( player_devaddr_t addr, StgDriver* driver, ConfigFile* cf, int section );
  virtual ~InterfacePtz( void ){ /* TODO: clean up*/ };

  virtual int ProcessMessage( QueuePointer & resp_queue,
			      player_msghdr * hdr,
			      void * data );
  virtual void Publish( void );
};

class InterfaceSonar : public InterfaceModel
{
 public:
  InterfaceSonar( player_devaddr_t addr, StgDriver* driver, ConfigFile* cf, int section );
  virtual ~InterfaceSonar( void ){ /* TODO: clean up*/ };

  virtual int ProcessMessage( QueuePointer & resp_queue,
			      player_msghdr * hdr,
			      void * data );
  virtual void Publish( void );
};


class InterfaceBumper : public InterfaceModel
{
 public:
  InterfaceBumper( player_devaddr_t addr, StgDriver* driver, ConfigFile* cf, int section );
  virtual ~InterfaceBumper( void ){ /* TODO: clean up*/ };

  virtual int ProcessMessage( QueuePointer & resp_queue,
			      player_msghdr * hdr,
			      void * data );
  virtual void Publish( void );
};

class InterfaceLocalize : public InterfaceModel
{
 public:
  InterfaceLocalize( player_devaddr_t addr,
		     StgDriver* driver,
		     ConfigFile* cf,
		     int section );

  virtual ~InterfaceLocalize( void ){ /* TODO: clean up*/ };

  virtual void Publish( void );
  virtual int ProcessMessage(QueuePointer & resp_queue,
                             player_msghdr_t* hdr,
                             void* data);
};

class InterfaceMap : public InterfaceModel
{
 public:
  InterfaceMap( player_devaddr_t addr, StgDriver* driver, ConfigFile* cf, int section );
  virtual ~InterfaceMap( void ){ /* TODO: clean up*/ };

  virtual int ProcessMessage( QueuePointer & resp_queue,
			      player_msghdr * hdr,
			      void * data );
  //virtual void Publish( void );

  // called by ProcessMessage to handle individual messages

  int HandleMsgReqInfo( QueuePointer & resp_queue,
			player_msghdr * hdr,
			void * data );
  int HandleMsgReqData( QueuePointer & resp_queue,
			player_msghdr * hdr,
			void * data );
};

class PlayerGraphics2dVis;
class InterfaceGraphics2d : public InterfaceModel
{
 public:
  InterfaceGraphics2d( player_devaddr_t addr, StgDriver* driver, ConfigFile* cf, int section );
  virtual ~InterfaceGraphics2d( void );

  void Subscribe(QueuePointer &queue);
  void Unsubscribe(QueuePointer &queue);

  virtual int ProcessMessage( QueuePointer & resp_queue,
			      player_msghdr * hdr,
			      void * data );

  PlayerGraphics2dVis * vis;
};

class PlayerGraphics3dVis;
class InterfaceGraphics3d : public InterfaceModel
{
 public:
  InterfaceGraphics3d( player_devaddr_t addr, StgDriver* driver, ConfigFile* cf, int section );
  virtual ~InterfaceGraphics3d( void );

  void Subscribe(QueuePointer &queue);
  void Unsubscribe(QueuePointer &queue);

  virtual int ProcessMessage( QueuePointer & resp_queue,
			      player_msghdr * hdr,
			      void * data );

  PlayerGraphics3dVis * vis;
};

/** Replaces Player's real time clock object */
class StTime : public PlayerTime
{
 private:
  StgDriver* driver;

 public:
 // Constructor
  StTime( StgDriver* driver );

 // Destructor
 virtual ~StTime();

 // Get the simulator time
 int GetTime(struct timeval* time);
 int GetTimeDouble(double* time);
};


#endif

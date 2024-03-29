/* worldgui.cc
    Implements a subclass of World that has an FLTK and OpenGL GUI
    Authors: Richard Vaughan (vaughan@sfu.ca)
             Alex Couture-Beil (asc17@sfu.ca)
             Jeremy Asher (jra11@sfu.ca)
    SVN: $Id: worldgui.cc 8329 2009-10-27 06:30:39Z rtv $
*/

/** @defgroup worldgui World with Graphical User Interface

The Stage window consists of a menu bar and a view of the simulated
world.

The world view shows part of the simulated world. You can zoom the
view in and out, and scroll it to see more of the world. Simulated
robot devices, obstacles, etc., are rendered as colored polygons. The
world view can also show visualizations of the data and configuration
of various sensor and actuator models. The View menu has options to
control which data and configurations are rendered.

API: Stg::WorldGui

<h2>Worldfile Properties</h2>

@par Summary and default values

speedup 1

@verbatim
window
(
  size [ 400 300 ]
  
  # camera options
  center [ 0 0 ]
  rotate [ 0 0 ]
  scale 1.0

  # perspective camera options
  pcam_loc [ 0 -4 2 ]
  pcam_angle [ 70 0 ]

  # GUI options
  show_data 0
  show_flags 1
  show_blocks 1
  show_clock 1
  show_footprints 0
  show_grid 1
  show_trailarrows 0
  show_trailrise 0
  show_trailfast 0
  show_occupancy 0
  show_tree 0
  pcam_on 0
  screenshots 0
)
@endverbatim

@par Details
 - speedup <int>\n
 Stage will attempt to run at this multiple of real time. If -1,
 Stage will run as fast as it can go, and not attempt to track real
 time at all. 

- size [ <width:int> <height:int> ]\n
size of the window in pixels
- center [ <x:float> <y:float> ]\n
location of the center of the window in world coordinates (meters)
- rotate [ <pitch:float> <yaw:float> ]\n
angle relative to straight up, angle of rotation (degrees)
- scale <float>\n
ratio of world to pixel coordinates (window zoom)
- pcam_loc [ <x:int> <y:int> <z:int> ]\n
location of the perspective camera (meters)
- pcam_angle [ <pitch:float> <yaw:float> ]\n
verticle and horizontal angle of the perspective camera
- pcam_on <int>\n
whether to start with the perspective camera enabled (0/1)


<h2>Using the Stage window</h2>


<h3>Scrolling the view</h3>
<p>Left-click and drag on the background to move your view of the world.

<h3>Zooming the view</h3>
<p>Scroll the mouse wheel to zoom in or out on the mouse cursor.

<h3>Saving the world</h3>
<P>You can save the current pose of everything in the world, using the
File/Save menu item. <b>Warning: the saved poses overwrite the currentworld file.</b> Make a copy of your world file before saving if you
want to keep the old poses.  Alternatively the File/Save As menu item
can be used to save to a new world file.

<h3>Pausing and resuming the clock</h3> <p>The simulation can be
paused or resumed by pressing the 'p' key. Run one simulation step at
a time by pressing the '.' (period) key. Hold down the '.' key to step
repeatedly. Stepping leaves the simulation paused, so press 'p' to
resume running. The initial paused/unpaused state can be set in the
worldfile using the "paused" property. 

<h3>Selecting models</h3> <p>Models can be selected by clicking on
them with the left mouse button.  It is possible to select multiple
models by holding the shift key and clicking on multiple models.
Selected models can be moved by dragging or rotated by holding the
right mouse button and moving the mouse. Selections can be cleared by
clicking on an empty location in the world.  After clearing the
selection, the last single model selected will be saved as the target
for several view options described below which affect a particular
model.

<h3>View options</h3>
<p>The View menu provides access to a number of features affecting how
the world is rendered.  To the right of each option there is usually
a keyboard hotkey which can be pressed to quickly toggle the relevant
option.

<p>Sensor data visualizations can be toggled by the "Data" option
(shortcut 'd').  The filter data option (shortcut shift-'d') opens a
dialog which enables turning on and off visualizations of particular
sensors.  The "Visualize All" option in the dialog toggles whether
sensor visualizations are enabled for all models or only the currently
selected ones.

<p>The "Follow" option keeps the view centered on the last selected model.

<p>The "Perspective camera" option switches from orthogonal viewing to perspective viewing. 

<h3>Saving a screenshot</h3>
<p> To save a sequence of screenshots of the world, select the "Save
screenshots" option from the view menu to start recording images and
then select the option from the menu again to stop.

*/

#include <FL/Fl_Image.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_File_Chooser.H>

#include <set>
#include <sstream>
#include <iomanip>
#include <algorithm> // for std::sort

#include "canvas.hh"
#include "region.hh"
#include "worldfile.hh"
#include "file_manager.hh"
#include "options_dlg.hh"
#include "config.h" // build options from CMake

using namespace Stg;

static const char* AboutText = 
	"\n" 
	"Part of the Player Project\n"
	"http://playerstage.org\n"
	"\n"
	"Copyright 2000-2008 Richard Vaughan,\n"
	"Brian Gerkey, Andrew Howard, Reed Hedges, \n"
	"Toby Collett, Alex Couture-Beil, Jeremy Asher \n"
	"and contributors\n" 
	"\n"
	"Distributed under the terms of the \n"
	"GNU General Public License v2";

static const char* MoreHelpText = 
  "http://playerstage.org\n"
  "\n"
  "has these resources to help you:\n"
  "\n"
  "\t* A user manual including API documentation\n"
  "\t* A bug and feature request tracking system\n"
  "\t* Mailing lists for users and developers\n"
  "\t* A Wiki"
  "\n\n"
  "The user manual is included with the Stage source code but\n"
  "is not built by default. To build the manual, run \"make\"\n"
  "in the directory \"docsrc\" to produce \"docsrc/stage/index.html\" .\n"
  "(requires Doxygen and supporting programs to be installed first).\n";
 
WorldGui::WorldGui(int W,int H,const char* L) : 
  Fl_Window(W,H,L ),
  canvas( new Canvas( this,0,30,W,H-30 ) ),
  drawOptions(),
  fileMan( new FileManager() ),
  speedup(1.0), // real time
  mbar( new Fl_Menu_Bar(0,0, W, 30)),
  oDlg( NULL ),
  pause_time( false ),	
  real_time_interval( sim_interval ),
  real_time_now( RealTimeNow() ),
  real_time_recorded( real_time_now ),
  timing_interval( 20 )
{
  Fl::scheme( "gtk+" );
  resizable(canvas);
  
  end();
  
  label( PROJECT );
  
  // make this menu's shortcuts work whoever has focus
  mbar->global();
  mbar->textsize(12);
  
  mbar->add( "&File", 0, 0, 0, FL_SUBMENU );
  mbar->add( "File/&Load World...", FL_CTRL + 'l', (Fl_Callback*)fileLoadCb, this, FL_MENU_DIVIDER );
  mbar->add( "File/&Save World", FL_CTRL + 's', (Fl_Callback*)fileSaveCb, this );
  mbar->add( "File/Save World &As...", FL_CTRL + FL_SHIFT + 's', (Fl_Callback*)WorldGui::fileSaveAsCb, this, FL_MENU_DIVIDER );
  
  mbar->add( "File/E&xit", FL_CTRL+'q', (Fl_Callback*)fileExitCb, this );
  
  mbar->add( "&View", 0, 0, 0, FL_SUBMENU );

  mbar->add( "View/Reset", ' ', (Fl_Callback*)resetViewCb, this );

  mbar->add( "View/Filter data...", FL_SHIFT + 'd', (Fl_Callback*)viewOptionsCb, this );
  canvas->createMenuItems( mbar, "View" );

  mbar->add( "Run", 0,0,0, FL_SUBMENU );
  mbar->add( "Run/Pause", 'p', (Fl_Callback*)pauseCb, this );
  mbar->add( "Run/One step", '.', (Fl_Callback*)onceCb, this, FL_MENU_DIVIDER );
  mbar->add( "Run/Faster", ']', (Fl_Callback*)fasterCb, this );
  mbar->add( "Run/Slower", '[', (Fl_Callback*)slowerCb, this, FL_MENU_DIVIDER  );
  mbar->add( "Run/Realtime", '{', (Fl_Callback*)realtimeCb, this );
  mbar->add( "Run/Fast", '}', (Fl_Callback*)fasttimeCb, this );
  
  mbar->add( "&Help", 0, 0, 0, FL_SUBMENU );
  mbar->add( "Help/Getting help...", 0,  (Fl_Callback*)moreHelptCb, this, FL_MENU_DIVIDER );
  mbar->add( "Help/&About Stage...", 0, (Fl_Callback*)helpAboutCb, this );
  
  callback( (Fl_Callback*)windowCb, this );	 
  
  show();	
}

WorldGui::~WorldGui()
{
  delete mbar;
  if ( oDlg )
    delete oDlg;
  delete canvas;
}


void WorldGui::Show()
{
  show(); // fltk
}

void WorldGui::Load( const char* filename )
{
  PRINT_DEBUG1( "%s.Load()", token );
	
  // needs to happen before StgWorld load, or we segfault with GL calls on some graphics cards
  Fl::check();

  fileMan->newWorld( filename );
  
  stg_usec_t load_start_time = RealTimeNow();
  
  World::Load( filename );
  
  // worldgui exclusive properties live in the top-level section
  int world_section = 0; 
  speedup = wf->ReadFloat( world_section, "speedup", speedup );    
  paused = wf->ReadInt( world_section, "paused", paused );
  
  // use the window section for the rest
  int window_section = wf->LookupEntity( "window" );
  
  if( window_section > 0 ) 
	 {			
		int width =  (int)wf->ReadTupleFloat(window_section, "size", 0, w() );
		int height = (int)wf->ReadTupleFloat(window_section, "size", 1, h() );
		size( width,height );
		size_range( 100, 100 ); // set min size to 100/100, max size to screen size
		
		// configure the canvas
		canvas->Load(  wf, window_section );
		// warn about unused WF lines
		wf->WarnUnused();
  
		std::string title = PROJECT;
		if ( wf->filename.size() ) {
		  // improve the title bar to say "Stage: <worldfile name>"
		  title += ": ";		
		  title += wf->filename;
		}
		label( title.c_str() );
	 }
 
  stg_usec_t load_end_time = RealTimeNow();
	
  if( debug )
    printf( "[Load time %.3fsec]\n", 
	    (load_end_time - load_start_time) / 1e6 );
  
  Show();
}

void WorldGui::UnLoad() 
{
  World::UnLoad();
}

bool WorldGui::Save( const char* filename )
{
  PRINT_DEBUG1( "%s.Save()", token );
  
  // worldgui exclusive properties live in the top-level section
  int world_section = 0; 
  wf->WriteFloat( world_section, "speedup", speedup );    
  wf->WriteInt( world_section, "paused", paused );

  // use the window section for the rest
  int window_section = wf->LookupEntity( "window" );
	
  if( window_section > 0 ) // section defined
    {
      wf->WriteTupleFloat( window_section, "size", 0, w() );
      wf->WriteTupleFloat( window_section, "size", 1, h() );
	    
      canvas->Save( wf, window_section );
	    
      // TODO - per model visualizations save 
    }
	
	World::Save( filename );
	
  // TODO - error checking
  return true;
}

static void UpdateCallback( WorldGui* world )
{	
  world->Update();
}

bool WorldGui::Update()
{ 
  if( speedup > 0 )
	 Fl::repeat_timeout( (sim_interval/1e6) / speedup, (Fl_Timeout_Handler)UpdateCallback, this );
  // else we're called by an idle callback

  //printf( "speedup %.2f timeout %.6f\n", speedup, timeout );
  
  // occasionally we measure the real time elapsing, for reporting the
  // run speed
  if( updates % timing_interval == 0 )
	 { 
		stg_usec_t timenow = RealTimeNow();	 
		real_time_interval = timenow - real_time_recorded; 
		real_time_recorded = timenow;
	 }   

  // inherit
  bool done = World::Update();

	if( Model::trail_length > 0 && updates % Model::trail_interval == 0 )
		FOR_EACH( it, active_velocity )
			(*it)->UpdateTrail();

  if( done )
    {
      quit_time = 0; // allows us to continue by un-pausing
      Stop();
    }
  
  return done;
}

std::string WorldGui::ClockString() const
{
  std::string str = World::ClockString();
  
  double localratio = (double)sim_interval / (double)(real_time_interval/timing_interval);
  
  char buf[32];
  snprintf( buf, 32, " [%.1f]", localratio );
  str += buf;
  
  if( paused == true )
	 str += " [ PAUSED ]";
  
  return str;
}


void WorldGui::AddModel( Model*  mod  )
{
  if( mod->parent == NULL )
	 canvas->AddModel( mod );
  
  World::AddModel( mod );
}


void WorldGui::RemoveChild( Model* mod )
{
  canvas->RemoveModel( mod );

  World::RemoveChild( mod );
}


std::string WorldGui::EnergyString()
{	
  char str[512]; 
  snprintf( str, 255, "Energy\n  stored:   %.0f / %.0f KJ\n  input:    %.0f KJ\n  output:   %.0f KJ at %.2f KW\n",
	    PowerPack::global_stored / 1e3,
	    PowerPack::global_capacity /1e3,
	    PowerPack::global_input / 1e3,
	    PowerPack::global_dissipated / 1e3,
	    (PowerPack::global_dissipated / (sim_time / 1e6)) / 1e3 );
  
  return std::string( str );
}

void WorldGui::DrawOccupancy()
{  
  FOR_EACH( it, superregions )
	 (*it).second->DrawOccupancy();
}

void WorldGui::DrawVoxels()
{  
  FOR_EACH( it, superregions )
	 (*it).second->DrawVoxels();
}

void WorldGui::windowCb( Fl_Widget* w, WorldGui* wg )
{
  switch ( Fl::event() ) {
  case FL_SHORTCUT:
    if ( Fl::event_key() == FL_Escape )
      return;
  case FL_CLOSE: // clicked close button
    bool done = wg->closeWindowQuery();
    if ( !done )
      return;
  }

  puts( "User closed window" );
  exit(0);
}

void WorldGui::fileLoadCb( Fl_Widget* w, WorldGui* wg )
{
  const char* filename;
  //bool success;
  const char* pattern = "World Files (*.world)";
	
	std::string worldsPath = wg->fileMan->worldsRoot();
	worldsPath.append( "/" );
  Fl_File_Chooser fc( worldsPath.c_str(), pattern, Fl_File_Chooser::CREATE, "Load World File..." );
  fc.ok_label( "Load" );
	
  fc.show();
  while (fc.shown())
    Fl::wait();
	
  filename = fc.value();
	
  if (filename != NULL) { // chose something
    if ( FileManager::readable( filename ) ) {
      // file is readable, clear and load

      // if (initialized) {
      wg->Stop();
      wg->UnLoad();
      // }
			
      // todo: make sure loading is successful
      wg->Load( filename );
      wg->Start(); // if (stopped)
    }
    else {
      fl_alert( "Unable to read selected world file." );
    }
		

  }
}

void WorldGui::fileSaveCb( Fl_Widget* w, WorldGui* wg )
{
  // save to current file
  bool success =  wg->Save( NULL );
  if ( !success ) {
    fl_alert( "Error saving world file." );
  }
}

void WorldGui::fileSaveAsCb( Fl_Widget* w, WorldGui* wg )
{
  wg->saveAsDialog();
}

void WorldGui::fileExitCb( Fl_Widget* w, WorldGui* wg ) 
{
  bool done = wg->closeWindowQuery();
  if (done) {
	 puts( "User exited via menu" );
    exit(0);
  }
}

void WorldGui::resetViewCb( Fl_Widget* w, WorldGui* wg )
{
  wg->canvas->current_camera->reset();
  
  if( Fl::event_state( FL_CTRL ) ) 
	 {
		wg->canvas->resetCamera();
	 }
  wg->canvas->redraw();
}

void WorldGui::slowerCb( Fl_Widget* w, WorldGui* wg )
{
  if( wg->speedup <= 0 )
	 {
		wg->speedup = 100.0;
		wg->SetTimeouts();
	 }
  else
	 wg->speedup *= 0.8;  
}

void WorldGui::fasterCb( Fl_Widget* w, WorldGui* wg )
{
  if( wg->speedup <= 0 )
	 putchar( 7 ); // bell - can go no faster
  else
	 wg->speedup *= 1.2;
}

void WorldGui::realtimeCb( Fl_Widget* w, WorldGui* wg )
{
  //puts( "real time" );
  wg->speedup = 1.0;

  if( !wg->paused ) 
	 wg->SetTimeouts();
}

void WorldGui::fasttimeCb( Fl_Widget* w, WorldGui* wg )
{
  //puts( "fast time" );
  wg->speedup = -1;
 
  if( !wg->paused ) 
	 wg->SetTimeouts();  
}

void WorldGui::Start()
{
  World::Start();
  
  // start the timer that causes regular redraws
  Fl::add_timeout( ((double)canvas->interval/1000), 
						 (Fl_Timeout_Handler)Canvas::TimerCallback, 
						 canvas );
  
  SetTimeouts();
}


void WorldGui::SetTimeouts()
{
  // remove the old callback, wherever it was
  Fl::remove_idle( (Fl_Timeout_Handler)UpdateCallback, this );	  
  Fl::remove_timeout( (Fl_Timeout_Handler)UpdateCallback, this );	  
  
  if( speedup > 0.0 ) 
	 // attempt some multiple of real time	 
	 Fl::add_timeout( (sim_interval/1e6) / speedup, (Fl_Timeout_Handler)UpdateCallback, this );
  else 
	 // go as fast as possible
	 Fl::add_idle( (Fl_Timeout_Handler)UpdateCallback, this );
}

void WorldGui::Stop()
{
  World::Stop();
  
  Fl::remove_timeout( (Fl_Timeout_Handler)Canvas::TimerCallback );	
  Fl::remove_timeout( (Fl_Timeout_Handler)UpdateCallback );	
  Fl::remove_idle( (Fl_Timeout_Handler)UpdateCallback, this );	  

  // drawn 'cos we cancelled the timeout
  canvas->redraw(); // in case something happened that will never be
										// drawn otherwise
}  

void WorldGui::pauseCb( Fl_Widget* w, WorldGui* wg )
{
  wg->TogglePause();
}

void WorldGui::onceCb( Fl_Widget* w, WorldGui* wg )
{
  //wg->paused = true;
  wg->Stop();

  // run exactly once
  wg->World::Update();
}

void WorldGui::viewOptionsCb( OptionsDlg* oDlg, WorldGui* wg ) 
{
  // sort the vector by option label alphabetically
  //std::sort();// wg->option_table.begin(), wg->option_table.end() );//, sort_option_pointer );
  //std::sort();// wg->option_table.begin(), wg->option_table.end() );//, sort_option_pointer );

  if ( !wg->oDlg ) 
	 {
		int x = wg->w()+wg->x() + 10;
		int y = wg->y();
		OptionsDlg* oDlg = new OptionsDlg( x,y, 180,250 );
		oDlg->callback( (Fl_Callback*)optionsDlgCb, wg );
		
		oDlg->setOptions( wg->option_table );
		oDlg->showAllOpt( &wg->canvas->visualizeAll );
		wg->oDlg = oDlg;
		oDlg->show();
	 }
  else 
	 {
		wg->oDlg->hide();
		delete wg->oDlg;
		wg->oDlg = NULL;
	 }
 
}

void WorldGui::optionsDlgCb( OptionsDlg* oDlg, WorldGui* wg ) 
{
  // get event from dialog
  OptionsDlg::event_t event;
  event = oDlg->event();
	
  // Check FLTK events first
  switch ( Fl::event() ) {
  case FL_SHORTCUT:
    if ( Fl::event_key() != FL_Escape ) 
      break; //return
    // otherwise, ESC pressed-> do as below
  case FL_CLOSE: // clicked close button
    // override event to close
    event = OptionsDlg::CLOSE;
    break;
  }
	
  switch ( event ) {
  case OptionsDlg::CHANGE: 
    {
      //Option* o = oDlg->changed();
      //printf( "\"%s\" changed to %d!\n", o->name().c_str(), o->val() );			
      break;
    }			
  case OptionsDlg::CLOSE:
    // invalidate the oDlg pointer from the Wg
    //   instance before the dialog is destroyed
    wg->oDlg = NULL; 
    oDlg->hide();
    Fl::delete_widget( oDlg );
    return;	
  case OptionsDlg::NO_EVENT:
  case OptionsDlg::CHANGE_ALL:
    break;
  }
}

void aboutOKBtnCb( Fl_Return_Button* btn, void* p ) 
{
  btn->window()->do_callback();
}

void aboutCloseCb( Fl_Window* win, Fl_Text_Display* textDisplay ) 
{
  Fl_Text_Buffer* tbuf = textDisplay->buffer();
  textDisplay->buffer( NULL );
  delete tbuf;
  Fl::delete_widget( win );
}

void WorldGui::helpAboutCb( Fl_Widget* w, WorldGui* wg ) 
{
  fl_register_images();
	
  const int Width = 420;
  const int Height = 330;
  const int Spc = 10;
  const int ButtonH = 25;
  const int ButtonW = 60;
  const int pngH = 82;
  //const int pngW = 264;
	
  Fl_Window* win = new Fl_Window( Width, Height ); // make a window

  Fl_Box* box = new Fl_Box( Spc, Spc, 
			    Width-2*Spc, pngH ); // widget that will contain image
	
  std::string fullpath;
  fullpath = FileManager::findFile( "assets/stagelogo.png" );
  Fl_PNG_Image* png = new Fl_PNG_Image( fullpath.c_str() ); // load image into ram
  box->image( png ); // attach image to box
	
  Fl_Text_Display* textDisplay;
  textDisplay = new Fl_Text_Display( Spc, pngH+2*Spc,
				     Width-2*Spc, Height-pngH-ButtonH-4*Spc );
  textDisplay->box( FL_NO_BOX );
  textDisplay->color( win->color() );
  win->callback( (Fl_Callback*)aboutCloseCb, textDisplay );
		
  Fl_Text_Buffer* tbuf = new Fl_Text_Buffer;
  tbuf->text( PROJECT );
  tbuf->append( "-" );
  tbuf->append( VERSION );
  tbuf->append( AboutText );
  //textDisplay->wrap_mode( true, 50 );
  textDisplay->buffer( tbuf );
	
  Fl_Return_Button* button;
  button = new Fl_Return_Button( (Width - ButtonW)/2, Height-Spc-ButtonH,
				 ButtonW, ButtonH,
				 "&OK" );
  button->callback( (Fl_Callback*)aboutOKBtnCb );
	
  win->show();
}

void WorldGui::moreHelptCb( Fl_Widget* w, WorldGui* wg ) 
{
  const int Width =  500;
  const int Height = 250;
  const int Spc = 10;
	
  Fl_Window* win = new Fl_Window( Width, Height ); // make a window
	  win->label( "Getting help with Stage" );

  Fl_Text_Display* textDisplay;
  textDisplay = new Fl_Text_Display( Spc, Spc,
												 Width-2*Spc, Height-2*Spc );

  win->resizable( textDisplay );
  textDisplay->box( FL_NO_BOX );
  textDisplay->color( win->color() );
		
  Fl_Text_Buffer* tbuf = new Fl_Text_Buffer;
  tbuf->append( MoreHelpText );
  // textDisplay->wrap_mode( true, 50 );
  textDisplay->buffer( tbuf );
	
  win->show();
}


bool WorldGui::saveAsDialog()
{
  const char* newFilename;
  bool success = false;
  const char* pattern = "World Files (*.world)";

  Fl_File_Chooser fc( wf->filename.c_str(), pattern, Fl_File_Chooser::CREATE, "Save File As..." );
  fc.ok_label( "Save" );

  fc.show();
  while (fc.shown())
    Fl::wait();

  newFilename = fc.value();

  if (newFilename != NULL) {
    // todo: make sure file ends in .world
    success = Save( newFilename );
    if ( !success ) {
      fl_alert( "Error saving world file." );
    }
  }

  return success;
}

bool WorldGui::closeWindowQuery()
{
  int choice;
	
  if ( wf ) {
    // worldfile loaded, ask to save
    choice = fl_choice("Quitting Stage",
		       "&Cancel", // ->0: defaults to ESC
		       "&Save, then quit", // ->1
		       "&Quit without saving" // ->2
		       );
		
    switch (choice) {
    case 1: // Save before quitting
      if ( saveAsDialog() ) 
	return true;      
      else 
	return false;
      
    case 2: // Quit without saving
      return true;
    }
		
    // Cancel
    return false;
  }
  else {
    // nothing is loaded, just quit
    return true;
  }
}

void WorldGui::DrawBoundingBoxTree()
{
  FOR_EACH( it, World::children )
    (*it)->DrawBoundingBoxTree();
}

void WorldGui::PushColor( Color col )
{ canvas->PushColor( col ); } 

void WorldGui::PushColor( double r, double g, double b, double a )
{ canvas->PushColor( r,g,b,a ); }

void WorldGui::PopColor()
{ canvas->PopColor(); }

Model* WorldGui::RecentlySelectedModel() const
{ return canvas->last_selection; }

stg_usec_t WorldGui::RealTimeNow() const
{
  struct timeval tv;
  gettimeofday( &tv, NULL );  // slow system call: use sparingly
  return( tv.tv_sec*1000000 + tv.tv_usec );
}

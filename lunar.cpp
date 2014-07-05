
#include <iostream>
#include <list>
#include <cstdlib>
#include <vector>
#include <sys/time.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include "lunar.h"
/*
 * Header files for X functions
 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>

using namespace std;

SpaceShip* spaceShip = new SpaceShip(-100,-50,0,0,25,25); //user space ship
Terrain terrain;
vector< Dock > LandingPads;
Explosion explosion;
Vision vision;
int enemyShipNumber=0;
SpaceShip* enemyShips;

void initX(int argc, char* argv[], XInfo& xinfo) {
	xinfo.display = XOpenDisplay( "" );
    if ( !xinfo.display ) {
        error( "Can't open display." );
    }
		
    xinfo.screen = DefaultScreen( xinfo.display );
    unsigned long background = WhitePixel( xinfo.display, xinfo.screen );
    unsigned long foreground = BlackPixel( xinfo.display, xinfo.screen );
	
	XSizeHints hints;	
	
	hints.x = 100;
    hints.y = 100;
    hints.width = maxScreenWidth;
    hints.height = maxScreenHeight;
    hints.flags = PPosition | PSize;
    xinfo.window = XCreateSimpleWindow( xinfo.display, DefaultRootWindow( xinfo.display ),
                                        hints.x, hints.y, hints.width, hints.height,
                                        Border, foreground, background );
    XSetStandardProperties( xinfo.display, xinfo.window, "Lunar Landing", "Lunar Landing", None,
                            argv, argc, &hints );


	int i = 0;
	xinfo.gc[i] = XCreateGC(xinfo.display, xinfo.window, 0, 0);
	XSetForeground(xinfo.display, xinfo.gc[i], WhitePixel(xinfo.display, xinfo.screen));
	XSetBackground(xinfo.display, xinfo.gc[i], BlackPixel(xinfo.display, xinfo.screen));
	XSetFillStyle(xinfo.display, xinfo.gc[i], FillSolid);
	XSetLineAttributes(xinfo.display, xinfo.gc[i],
	                     1, LineSolid, CapButt, JoinRound);

	i = 1;
	xinfo.gc[i] = XCreateGC(xinfo.display, xinfo.window, 0, 0);
	XSetForeground(xinfo.display, xinfo.gc[i], BlackPixel(xinfo.display, xinfo.screen));
	XSetBackground(xinfo.display, xinfo.gc[i], WhitePixel(xinfo.display, xinfo.screen));
	XSetFillStyle(xinfo.display, xinfo.gc[i], FillSolid);
	XSetLineAttributes(xinfo.display, xinfo.gc[i],
	                     1, LineSolid, CapButt, JoinRound);
	
	//setup pixmap
	int depth = DefaultDepth(xinfo.display, DefaultScreen(xinfo.display));
	xinfo.pixmap = XCreatePixmap(xinfo.display, xinfo.window, hints.width, hints.height, depth);
	xinfo.width = hints.width;
	xinfo.height = hints.height;

    // Tell the window manager what input events you want.
    XSelectInput( xinfo.display, xinfo.window,
                  ButtonPressMask | KeyPressMask |
                  ExposureMask | ButtonMotionMask | StructureNotifyMask);

    /*
     * Put the window on the screen.
     */
    XMapRaised( xinfo.display, xinfo.window );

	XFlush(xinfo.display);
	sleep(2);	// let server get set up before sending drawing commands
}

void toggleExtraMode(XInfo& xinfo){
	if (extraMode=!extraMode){
		vision.paint(xinfo);
		XSetClipRectangles(xinfo.display, xinfo.gc[1], 0, 0, vision.getVisionObject(), 1, Unsorted);	
	}
	else{
		XSetClipMask(xinfo.display, xinfo.gc[1], None);
	}
}

void explode(){
	spaceShip->setVy(0);
	spaceShip->setVx(0);
	spaceShip->setCollision(true);
	explosion.setXY(spaceShip->getX()+spaceShip->getWidth()/2, spaceShip->getY()+spaceShip->getHeight()/2);
}

//check the collision for the enemyship;
bool checkCollisionEnemyShip(){
	SpaceShip enemyShip;
	for (int i=0; i < enemyShipNumber; ++i){
		enemyShip = *(enemyShips+i);
		if (!((enemyShip.getX()+enemyShip.getWidth() < spaceShip->getX() || enemyShip.getX() > spaceShip->getX()+spaceShip->getWidth()) ||
			(enemyShip.getY()+enemyShip.getHeight() < spaceShip->getY() || enemyShip.getY() > spaceShip->getY()+spaceShip->getHeight()))){
			(enemyShips+i)->setCollision(true);
			(enemyShips+i)->setVx(0);		
			return true;
		}
	}
	return false;
}


//returns 0 for no collision
//returns 1 for collision
//returns 2 for sucessfully landed on dock
int isCollision (){
	//checking for collision on landing pad
	for(std::vector<Dock>::iterator landingPad = LandingPads.begin(); landingPad != LandingPads.end(); ++landingPad) {
		if (landingPad->getX() <= spaceShip->getX()  &&  spaceShip->getX() <= landingPad->getX()+landingPad->getWidth() &&
			landingPad->getX() <= spaceShip->getX()+spaceShip->getWidth() && spaceShip->getX()+spaceShip->getWidth() <= landingPad->getX()+landingPad->getWidth() &&
			landingPad->getY() <= spaceShip->getY()+spaceShip->getHeight() /*&& spaceShip->getY()+spaceShip->getHeight() <= landingPad->getY() + landingPad->getHeight()*/){
			if (spaceShip->getVy() > 2){
				explode();
				return collision;
			}
			else{
				spaceShip->setSucessfulLanding(true);
				return successfullyLanded;
			}	
		}
	}
	//check for collision on terrain
	
	vector< XPoint > terrainPoints = terrain.getTerrainPoints();
	std::vector<XPoint>::iterator point2 = terrainPoints.begin();
	while( point2 != terrainPoints.end()) {
		if (spaceShip->getX() <= point2->x && point2->x <= spaceShip->getX()+spaceShip->getWidth() &&
			spaceShip->getY() <= point2->y && point2->y <= spaceShip->getY()+spaceShip->getWidth()){
			explode();
			return collision;
		}
		XPoint point1 = *point2;
		//at the last point, break
		if (++point2 == terrainPoints.end()) break;	
		// check if the points are in range for y
		if (point1.y > spaceShip->getY()+spaceShip->getHeight() && point2->y > spaceShip->getY()+spaceShip->getHeight()){		 	
			continue;
		}
		//check if points are out of ranged for x
		if ((point1.x < spaceShip->getX() && point2->x < spaceShip->getX()) ||
			(point1.x > spaceShip->getX()+spaceShip->getWidth() && point2->x > spaceShip->getX()+spaceShip->getWidth())){	
			continue;		
		}	
		double changeInX = (point2->x - point1.x);
		if (changeInX == 0 ){
			explode();
			return collision;
		}
		double slope =  (point2->y - point1.y) / changeInX;
		double b = point2->y - slope*point2->x;
		double collisionHeight1=slope*spaceShip->getX() + b;
		double collisionHeight2=slope*(spaceShip->getX() + spaceShip->getWidth()) + b;

		if (collisionHeight1>0 && collisionHeight1 <= spaceShip->getY()+spaceShip->getHeight()){
			explode();
			return collision;
		}
		else if (collisionHeight2>0 && collisionHeight2 <= spaceShip->getY()+spaceShip->getHeight()){
			explode();
			return collision;
		}
	}
	if (extraMode){
		if(checkCollisionEnemyShip()){
			explode();
		}
	}
	return no_collision;
} 

// get microseconds
unsigned long now() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}

void updateShipPosition(){
	if (spaceShip->getVx() != 0){
		if (spaceShip->getX()+spaceShip->getVx()+spaceShip->getWidth() > maxScreenWidth){
			spaceShip->setX(maxScreenWidth-spaceShip->getWidth());
			spaceShip->setVx(0);
		}
		else if (spaceShip->getX()+spaceShip->getVx() < 0){
			spaceShip->setX(0);
			spaceShip->setVx(0);
		}
		else spaceShip->setX(spaceShip->getX()+spaceShip->getVx());
	} 
	if (spaceShip->getVy() != 0){
		if (spaceShip->getY()+spaceShip->getVy()+spaceShip->getHeight() > maxScreenHeight){
			spaceShip->setY(maxScreenHeight-spaceShip->getHeight());
			spaceShip->setVy(0);
		}
		else if (spaceShip->getY()+spaceShip->getVy() < 0){
			spaceShip->setY(0);
			spaceShip->setVy(0);		
		}
		else spaceShip->setY(spaceShip->getY()+spaceShip->getVy());
	} 
}

void moveShip (int key) {
 	switch (key) {
		case XK_Up:
			spaceShip->setVy(spaceShip->getVy()-unit/resistance);
			break;
		case  XK_Left:
			if (extraMode) spaceShip->setVx(spaceShip->getVx()+unit > max_accerlation_Vy ? max_accerlation_Vy : spaceShip->getVx()+unit);
			else spaceShip->setVx(spaceShip->getVx()-unit < -max_accerlation_Vy ? -max_accerlation_Vy : spaceShip->getVx()-unit);
			break;
		case XK_Right:
			if(extraMode) spaceShip->setVx(spaceShip->getVx()-unit < -max_accerlation_Vy ? -max_accerlation_Vy : spaceShip->getVx()-unit);
			else spaceShip->setVx(spaceShip->getVx()+unit > max_accerlation_Vy ? max_accerlation_Vy : spaceShip->getVx()+unit);
			break;
		case  XK_Down:
		default:
			spaceShip->setVy(spaceShip->getVy()+unit);
			break;
	}
	if(extraMode){
		spaceShip->useFuel();
	}
	updateShipPosition();

}


void updateEnemyShip(){

	for (int i=0; i < enemyShipNumber; ++i){
		if((enemyShips+i)->getCollision()) continue;
		(enemyShips+i)->setX((enemyShips+i)->getX()+(enemyShips+i)->getVx()); 
		if ((enemyShips+i)->getX()+(enemyShips+i)->getWidth() >= maxScreenWidth || (enemyShips+i)->getX()+(enemyShips+i)->getWidth()<= 0){
			(enemyShips+i)->flipShip();
		}
	}
}
void repaint(XInfo& xinfo){
		if (extraMode && !spaceShip->getCollision() && !spaceShip->getSucessfulLanding()){
			int x = spaceShip->getX() + spaceShip->getWidth()/2  - vision.getVisionRange()/2;
			int y = spaceShip->getY() + spaceShip->getHeight()/2 - vision.getVisionRange()/2;
			if (x < 0) x=0;
			else if (x+vision.getVisionRange() > maxScreenWidth) x= maxScreenWidth - vision.getVisionRange();
			if (y < 0) y=0;
			else if (y+vision.getVisionRange() > maxScreenHeight) y= maxScreenHeight - vision.getVisionRange();
			spaceShip->rePostionFuelBar(x+5,y+5);
			vision.setX(x); //relocate the vision range
			vision.setY(y);		
			vision.paint(xinfo);
		}
		else {
			XFillRectangle(xinfo.display, xinfo.pixmap, xinfo.gc[0], 
				0, 0, xinfo.width, xinfo.height);
		}
		if(shouldDraw){
			spaceShip->paint(xinfo);
			terrain.paint(xinfo);
			if (spaceShip->getCollision())drawMenuInstruction("CRASHED!",xinfo);
			if (extraMode){
				if (spaceShip->getCollision()){
					if(explosion.getRadius() < ExplosionSize) explosion.paint(xinfo);
					else explosion.setExplosionEnded(true);
				}
				for (int i=0; i < enemyShipNumber; ++i){
					if((enemyShips+i)->getCollision()) continue;
					(enemyShips+i)->paint(xinfo);
					(enemyShips+i)->randomChangeSpeed();
					(enemyShips+i)->randomChangeDirection();
				}
				for (int i=0; i <otherExplosionsLocations.size(); ++i ){
					if(otherExplosionsLocations[i]->getRadius() < ExplosionSize) otherExplosionsLocations[i]->paint(xinfo);
					else otherExplosionsLocations[i]->setExplosionEnded(true);				
				}
			
			}
			for(std::vector<Dock>::iterator landingPad = LandingPads.begin(); landingPad != LandingPads.end(); ++landingPad) {
					landingPad->paint(xinfo);
			}
				
			XCopyArea(xinfo.display, xinfo.pixmap, xinfo.window, xinfo.gc[0], 
			0, 0, xinfo.width, xinfo.height,  // region of pixmap to copy
			rePositionX, rePositionY); // position to put top left corner of pixmap in window
	
		}
		XFlush(xinfo.display);
}

int generateLandingPadsAndTerrain(int screenHeight, int screenWidth){
	terrain.reset();
	LandingPads.clear();
	int numberOfLandingPads = my_rand(2,maxLandingPads);
	const int landingPadWidth = spaceShip->getWidth()*3;
	const int landingPadHeight = spaceShip->getHeight()*1/2;
	const int minY = minTerrainScaleY;
	const int maxY = maxTerrainScaleY;
	int spacing = (screenWidth- landingPadWidth)/numberOfLandingPads -landingPadWidth;
	
	int firstValidPositionOfLandingPad = 0; //the location of x corrd of the valid landing pad
	int lastPadLocation =0 ; //the x coord of the last landing pad (note start at zero, so we draw the terrain at x = 0)
	Dock landingPad;
	for (int i=0; i < numberOfLandingPads; ++i){
		//generate landing pad		
		
		landingPad.setX(my_rand(i*spacing+landingPadWidth, (i+1)*spacing));
		
		
		landingPad.setY(my_rand(screenHeight*3/4, screenHeight)-landingPadHeight);
		landingPad.setWidth(landingPadWidth);
		landingPad.setHeight(landingPadHeight);
		if (landingPad.getX() < firstValidPositionOfLandingPad || firstValidPositionOfLandingPad+landingPad.getWidth() > screenWidth ) break;
		firstValidPositionOfLandingPad = (i+1)*spacing;	

		//generate terrain
		terrain.generateTerrian(my_rand(minTerrainScaleX,maxTerrainScaleX), landingPad.getX(), landingPad.getY(), landingPad.getWidth(), lastPadLocation, minY,maxY); 
		LandingPads.push_back(landingPad);
		lastPadLocation = landingPad.getX()+landingPad.getWidth();
	}
	//connect the last ladnding pad to the screen width
	terrain.generateTerrian(my_rand(minTerrainScaleX,maxTerrainScaleX), screenWidth, my_rand(minY, maxY), 0, lastPadLocation, minY,maxY); 
}



void drawMenuInstruction(string gameStatus, XInfo& xinfo, int buffered){
	Message msg[6];
	msg[0].setString(gameStatus);
	msg[1].setString("Instructions");
	msg[2].setString("SPACE to start the game or pause the game");
	msg[3].setString("q to quit the game and exit the application");
	msg[4].setString("arrow keys to activate thrusters ");
	msg[5].setString("press w to toggle extra mode");
	for (int i = 0 ; i < 6; ++i){
		msg[i].setY(50+i*30);
		msg[i].setX(300);
		if (buffered)msg[i].paint(xinfo);
		else msg[i].paintMainScreen(xinfo);
	}

}

void drawMenuInstruction(string gameStatus, XInfo& xinfo){
	drawMenuInstruction(gameStatus,xinfo, true);
}

// update width and height when window is resized
//the state tell it to draw the menu or the current game
//state == string => menu page
//state == NULL => game interface
void handleResize(XInfo &xinfo, XEvent &event, string state) {
	XConfigureEvent xce = event.xconfigure;
	if (xce.width != xinfo.width || xce.height != xinfo.height) {
		if (xce.width < maxScreenWidth || xce.height < maxScreenHeight) {
			Message msg;
			msg.setString("Too Small");
			msg.setX(xce.width/2);
			msg.setY(xce.height/2);
			msg.paint(xinfo);
			shouldDraw= false;
		}else {
			shouldDraw= true;
			XFillRectangle(xinfo.display, xinfo.window, xinfo.gc[0],0, 0, xinfo.width, xinfo.height);
			XWindowAttributes windowInfo;
			XGetWindowAttributes(xinfo.display, xinfo.window, &windowInfo);
			rePositionX = abs((xce.width - xinfo.width))/2;
			rePositionY = abs((xce.height - xinfo.height))/2;
			repaint(xinfo); 
			drawMenuInstruction(state,xinfo);
			
		}
	}
}

int handleKeyPress(XInfo xinfo,XEvent event, bool & pause){
	if(shouldDraw){	
		KeySym key;
		char text[BufferSize];
		int i = XLookupString( (XKeyEvent*)&event, text, BufferSize, &key, 0 );
		if (key == XK_q ||  key == XK_Q) return 0;
		else if (key == XK_space){
			if (spaceShip->getCollision()) return 3;
			else if (extraMode){
				XDrawImageString(xinfo.display,xinfo.window, xinfo.gc[1], spaceShip->getX(), spaceShip->getY()-40, "Game is Paused!",15 );		
				pause=!pause;
			} 
			else {
				drawMenuInstruction("Game is Paused",xinfo,false);	
				pause=!pause;
			}
		}
		else if (key == XK_w  || key == XK_W){
			toggleExtraMode(xinfo);
		}
		else if (!pause && !spaceShip->getCollision()){
			if (extraMode){
				if (spaceShip->getFuel() > 0)moveShip(key); 
			}
			else moveShip(key); 
		}
			
	}
	return 1;
}

int startGame(XInfo& xinfo) {
	XEvent event;
	int result = 0;
	bool pause = false;
	unsigned long lastRepaint = 0;
	unsigned long accerlation = 0;

	spaceShip->resetSpaceShip();
	
	enemyShipNumber=my_rand(minEnemyShip, maxEnemyShip);
	enemyShips = new SpaceShip[enemyShipNumber];
	
	for (int i =0; i < enemyShipNumber; ++i){
		(enemyShips+i)->resetEnemyShip();
	}
	
	explosion.reset();
	explosionLocationReset(enemyShips,enemyShipNumber);
	generateLandingPadsAndTerrain(maxScreenHeight, maxScreenWidth);
	repaint(xinfo);
 	while( result  == 0) {
		if (XPending(xinfo.display) > 0) {
			XNextEvent( xinfo.display, &event );
			switch( event.type ) {
				case Expose:
					XFlush(xinfo.display);
			    	break;
				case KeyPress:
					{
					int flag =handleKeyPress(xinfo,event,pause);
					if (flag==0) return 0;
					else if (flag==3) return 3;
					}
			    	break;
				case ConfigureNotify:
					handleResize(xinfo, event,"");
					break;	
			}
		}
		if (!pause && shouldDraw) {
			// animation timing
			unsigned long end = now();

			if (end - accerlation > 100000 && !spaceShip->getCollision()) {
				if (spaceShip->getVy() + (double)1/gravity <= max_accerlation_Vy) spaceShip->setVy(spaceShip->getVy() + (double) 1/gravity);
				if (spaceShip->getVx() > 0 && spaceShip->getVx() - (double)1/drift >= 0) spaceShip->setVx(spaceShip->getVx() - (double) 1/drift);
				if (spaceShip->getVx() < 0 && spaceShip->getVx() + (double)1/drift <= 0) spaceShip->setVx(spaceShip->getVx() + (double) 1/drift);
				accerlation = now();
			
			}

			if (end - lastRepaint > 1000000/FPS) {
				updateShipPosition();
				if (!spaceShip->getCollision()) result = isCollision();
				if (extraMode ){
					if(spaceShip->getCollision()){
						result=0;
						//result= explosion.getExplosionEnded();
						XSetClipMask(xinfo.display, xinfo.gc[1], None);
					}else if (spaceShip->getSucessfulLanding()) XSetClipMask(xinfo.display, xinfo.gc[1], None);
					else{
						XSetClipRectangles(xinfo.display, xinfo.gc[1], 0, 0, vision.getVisionObject(), 1, Unsorted);
					}
					updateEnemyShip();
					
				}
				repaint(xinfo);
				lastRepaint = now();
			}

		
			// give the system time to do other things
			if (XPending(xinfo.display) == 0) {
				usleep(1000000/FPS - (end - lastRepaint));
			}
		}

	}
	return result;
	
}



//return 1 if game is over 0 other wise
int handleMenuKeys(XInfo xinfo, XEvent event, KeySym& key, string& gameMenuMsg){
	char text[BufferSize];
	int i = XLookupString( (XKeyEvent*)&event, text, BufferSize, &key, 0 );
	otherExplosionsLocationsReset();
	if (key == XK_w || key == XK_W){
		toggleExtraMode(xinfo);
	}	

	while (key == XK_space && shouldDraw) {
		switch (startGame(xinfo)) {
			case 1:
			gameMenuMsg="CRASHED!";
			drawMenuInstruction(gameMenuMsg,xinfo);
			return 0;
			break;
			case 2:
			gameMenuMsg="SUCCESSFULLY LANDED!";
			drawMenuInstruction(gameMenuMsg,xinfo);
			return 0;
			break;
			case 3:
				otherExplosionsLocationsReset();
			break;
			default:
			cout<<"Q pressed, exiting the game!"<<endl;
			return 1;
			break;
		}
	}
	return 0;
}

void menu (XInfo& xinfo){
	XEvent event;
	KeySym key = 0;
	XFillRectangle(xinfo.display,xinfo.pixmap, xinfo.gc[0], 0, 0, xinfo.width,xinfo.height);
	drawMenuInstruction("",xinfo);
	XCopyArea(xinfo.display, xinfo.pixmap, xinfo.window, xinfo.gc[1], 
			0, 0, xinfo.width, xinfo.height,  // region of pixmap to copy
			rePositionX, rePositionY); // position to put top left corner of pixmap in window
	XFlush(xinfo.display);
	string gameMenuMsg=" ";
	while( key != XK_q &&  key != XK_Q ) {
    	XNextEvent( xinfo.display, &event );
		switch( event.type ) {
			case KeyPress:
			if (handleMenuKeys(xinfo,event,key,gameMenuMsg)) return;
	       	break;
		case ConfigureNotify:
			handleResize(xinfo, event,gameMenuMsg);
			break;	
		}

	}

}

int main (int argc, char * argv[]){
	XInfo xinfo;
	initX(argc, argv, xinfo);
	menu(xinfo);
	delete spaceShip;
	delete enemyShips;
	otherExplosionsLocationsReset();
	XCloseDisplay(xinfo.display);
}

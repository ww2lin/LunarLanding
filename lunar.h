#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cstdlib>
#include <iostream>
#include <stdio.h>
#include <vector>

using namespace std;
class Explosion;
class SpaceShip;

//these should not be configured
bool shouldDraw = true; // set false to not draw anything onto the screen
const int no_collision = 0;
const int collision = 1;
const int successfullyLanded = 2;
const int maxScreenWidth = 800;
const int maxScreenHeight = 600;

vector< Explosion*> otherExplosionsLocations;
vector <SpaceShip*> explosionLocation;
//these can be configured
bool extraMode = false;
const int Border = 5;
const int BufferSize = 10;
const int gravity =6; //smaller the number smaller the gravitiy
const int unit = 4;
const int FPS = 60;
const int max_accerlation_Vy = 10;   //max accerlation for Vy
const int max_accerlation_Vx = 10;
const int drift = 5; //large the number large the drift
const int resistance = 2; // the oppose force

const int spaceShipWidth = 25;
const int spaceShipHeight = 25;
const int fuelUsage = 2;

int minTerrainScaleX = 3;
int maxTerrainScaleX = 15;
//changes when rescaling screen
int minTerrainScaleY = 300;
int maxTerrainScaleY = 600;
int rePositionX = 0; //reposition screen when, window is streched
int rePositionY = 0;
const int maxLandingPads = 5;
const int landGap=50;
const int ExplosionSize = 250;
const int totalFuel = 50;
const int minEnemyShip =3;
const int maxEnemyShip =6;
const int maxEnemyShipSpeed = 4;
bool clipping = true;
const int maxVisionRangeWidth = spaceShipWidth*10;
const int maxVisionRangeHeight = spaceShipWidth*10;
const int ExplosionPiecesSize = 3;


// pick a random int between a and b
int my_rand(int a, int b)
{
  return a + (b - a) * (double)rand() / RAND_MAX;

}


struct XInfo{
	Display* display;
	Window window;
	GC gc[2];
	int screen;
	Pixmap	pixmap;		// double buffer
	int		width;		// size of pixmap
	int		height;
};

void error (string str){
	cout <<str<<endl;
	exit(0);
}


/*
 * An abstract class representing displayable things.
 */
class Displayable {
public:
    virtual void paint(XInfo& xinfo) = 0;
};

/*
 * A text displayable
 */
class SpaceShip : public Displayable {
	class Fuel {
		public:
			int x;
			int y;
			int width;
			int height;
			int fuel;
			Fuel(){
				x=10;
				y=10;
				width=0;
				height=10;
				fuel=totalFuel;
			}
			void reset(){
				fuel=totalFuel;	
			}
			void decrementFuel(){
				if (fuel-fuelUsage >= 0){
					fuel-=fuelUsage;
					if (fuel < 0) fuel =0;
				}
			}
			int getFuelUsage(){
				return width + fuel;
			}
			int getFuel(){
				return fuel;
			}
			void setFuel(int fuel){
				this->fuel=fuel;
			}
			void rePostionFuelBar(int x,int y){
				this->x=x;
				this->y=y;
			}
	};
	int x;  //position on the view
	int y;
	int width;
	int height;
	double Vx;  //velocity of the ship
	double Vy;
	Fuel sFuel;  //spaceship fuel
	bool collision;
	bool isUserSpaceShip;
	bool sucessfulLanding; //enable when user sucesfully lands a landingpad. which then enable the whole screen
public:
	virtual void paint(XInfo& xinfo) {
		if (extraMode){
			if (!collision){
				if(isUserSpaceShip && !sucessfulLanding){
					XDrawImageString(xinfo.display, xinfo.pixmap, xinfo.gc[1], sFuel.x, sFuel.y+10, "Fuel:", 5 );
					XFillRectangle(xinfo.display,xinfo.pixmap, xinfo.gc[1], sFuel.x+40, sFuel.y,sFuel.getFuelUsage(),sFuel.height);	
				}
				XFillRectangle(xinfo.display,xinfo.pixmap, xinfo.gc[1], this->x, this->y,this->width,this->height);	
			}
		} else XFillRectangle(xinfo.display,xinfo.pixmap, xinfo.gc[1], this->x, this->y,this->width,this->height);
	}
	// constructor
	SpaceShip(int x, int y):x(x), y(y)  {}
	SpaceShip(){}
	SpaceShip(int x, int y, double Vx, double Vy, int width, int height):x(x), y(y), Vx(Vx), Vy(Vy),width(width),height(height)  {}
	void setX(int x){
		this->x = x;
	}
	int getX(){
		return this->x;	
	}
	void setY(int y){
		this->y = y;
	}
	int getY(){
		return this->y;	
	}
	void setVx(double Vx){
		this->Vx = Vx;
	}
	double getVx(){
		return this->Vx;	
	}
	void setVy(double Vy){
		this->Vy = Vy;
	}
	double getVy(){
		return this->Vy;	
	}
	void setWidth(int Width){
		this->width = width;
	}
	int getWidth(){
		return this->width;	
	}
	void setHeight(int height){
		this->height = height;
	}
	int getHeight(){
		return this->height;	
	}
	void useFuel(){
		sFuel.decrementFuel();
	}
	int getFuel(){
		return sFuel.getFuel();
	}
	void setCollision(bool collision){
		this->collision = collision;
	}
	bool getCollision(){
		return collision;
	}
	void rePostionFuelBar(int x, int y){
		sFuel.rePostionFuelBar(x,y);
	}
	void resetSpaceShip(){
		x=340;
		y=10;
		Vx=0;
		Vy=1/gravity;
		width=spaceShipWidth;
		height=spaceShipHeight;
		sFuel.reset();
		collision= false;
		isUserSpaceShip=true;
		sucessfulLanding=false;
	}
	void setSucessfulLanding(bool sucessfulLanding){
		this->sucessfulLanding=sucessfulLanding;
	}
	bool getSucessfulLanding(){
		return sucessfulLanding;
	}
	void resetEnemyShip(){
		width=spaceShipWidth/2;
		height=spaceShipHeight/2;
		x=my_rand(0,maxScreenWidth-width);
		y=my_rand(50+spaceShipHeight,maxScreenHeight-height);
		int direction =my_rand(0,2);
		if (direction){
			Vx=my_rand(1,maxEnemyShipSpeed);
		} else 	Vx= - my_rand(1,maxEnemyShipSpeed);
		Vy=0;
		sFuel.setFuel(0);
		isUserSpaceShip=false;
		collision= false;
	}
	//use to flip the dicretion of the ship
	void flipShip(){
		Vx=-Vx;
	}
	//use to change the spped
	void randomChangeSpeed(){
		if (random() % 100 == 0 ){
			if(Vx>0) Vx=my_rand(1,maxEnemyShipSpeed);
			else  Vx= -my_rand(1,maxEnemyShipSpeed);
		} 
	}
	void randomChangeDirection(){
		if (random() % 1000 == 0 ){
			Vx=-Vx;
		} 
	}

};

int enemySpaceShipCollision(vector<SpaceShip*>& explosionLocation,int x, int pos, int neg){
	int index=0;
	for (int i=0; i <explosionLocation.size(); ++i ){		
		int sX =  explosionLocation[i]->getX();
		int sY = explosionLocation[i]->getY();
		int sW= sX + explosionLocation[i]->getWidth();
		int sH = sY + explosionLocation[i]->getHeight();
		if (!(x+ExplosionPiecesSize < sX  || x  > sW  || pos+ExplosionPiecesSize < sY || sH < pos )) return index;
		else if (!(x+ExplosionPiecesSize < sX  || x  > sW  || neg+ExplosionPiecesSize < sY|| sH < neg)) return index;		
		++index;
	}
	return -1;
}


class Explosion  {
	int radius; //distance
 	int x; //offset to the ship location
	int y;
	int explosionEnded;
public:
	Explosion() {
		radius=0;
		explosionEnded=false;
	}
	void paint(XInfo& xinfo) {
		if(explosionEnded) return;
		for (int i=-radius; i <=radius; ++i){
			int point = getYPlot(i);
			if (point <0 ) continue;
			XFillRectangle(xinfo.display,xinfo.pixmap, xinfo.gc[1], i+x, point+y,ExplosionPiecesSize,ExplosionPiecesSize);
			XFillRectangle(xinfo.display,xinfo.pixmap, xinfo.gc[1], i+x, -point+y,ExplosionPiecesSize,ExplosionPiecesSize);
			int index =enemySpaceShipCollision(explosionLocation,i+x,point+y,-point+y) ;			
			if (index>=0) {
				Explosion * e = new Explosion();
				e->x =explosionLocation[index]->getX() + explosionLocation[index]->getWidth()/2;
				e->y =explosionLocation[index]->getY() + explosionLocation[index]->getHeight()/2;
				otherExplosionsLocations.push_back(e);
				
				SpaceShip* os = explosionLocation[index];
				os->setCollision(true);
				os->setVx(0);
				explosionLocation.erase(explosionLocation.begin() + index);
			}
		}
		++radius;
	}
	int getYPlot(int x){
		int ans = radius*radius  - x*x;
		if (ans < 0 ) return -1;
		return sqrt(ans);
	}
	void setX(int x){
		this->x=x;
	}
	void setY(int y){
		this->y=y;
	}
	void setXY(int x,int y){
		this->x = x;
		this->y = y;
	}
	int getRadius(){
		return radius;
	}
	void setExplosionEnded(bool explosionEnded){
		this->explosionEnded=explosionEnded;
	}
	bool getExplosionEnded(){
		return explosionEnded;
	}
	void reset(){
		explosionEnded=false;
		radius = 0;
		x=0;
		y=0;
		
		
	}
};

void explosionLocationReset (SpaceShip* enemyShips, int size){
	explosionLocation.clear();
	for (int i =0; i < size ; ++i){
		explosionLocation.push_back(enemyShips+i);
	}
}

void otherExplosionsLocationsReset(){
	for (int i=0; i <otherExplosionsLocations.size(); ++i){
		delete otherExplosionsLocations[i];
	}
	otherExplosionsLocations.clear();
}

class Dock : public Displayable {
public:
    virtual void paint(XInfo& xinfo) {
		XFillRectangle(xinfo.display,xinfo.pixmap, xinfo.gc[1], this->x, this->y,this->width,this->height);
    }
	Dock(int x, int y,int width, int height): x(x), y(y), width(width), height(height) {}
		void setX(int x){
		this->x = x;
	}
	Dock(){
		setPosition(0,0);
		width = 0;
		height = 0;
	}	
	int getX(){
		return this->x;	
	}
	void setY(int y){
		this->y = y;
	}
	int getY(){
		return this->y;	
	}
	void setPosition(int x, int y){
		this->x = x;
		this->y = y;
	}
	void setSize(int width, int height){
		this->width = width;
		this->height = height;
	}
	void setWidth(int width){
		this->width = width;
	}
	int getWidth(){
		return this->width;	
	}
	void setHeight(int height){
		this->height = height;
	}
	int getHeight(){
		return this->height;	
	}
private:
	int x;
	int y;
	int width;
	int height;
};


class Terrain : public Displayable {

	vector< XPoint > points;
	int space;

public:
	void generateTerrian(int n, int landingPad_x, int landingPad_y, int landingPad_width, int start_at, int minY, int maxY) {
		space = landingPad_x/(my_rand(minTerrainScaleX,maxTerrainScaleX));
		
		XPoint p;
		p.x = start_at;
		p.y = my_rand(minY, maxY);
		points.push_back(p);
		for (int i=0; i < n; i++) {
		    p.x = my_rand(start_at+i*space, start_at+(i+1)*space)+landGap;
		    p.y = my_rand(minY, maxY);
				if (p.x > landingPad_x) break;
		    points.push_back(p);
			
		}
		p.x = landingPad_x;  //connect to landing pad //note this the point of top left corner
		p.y = landingPad_y;
		points.push_back(p);
		p.x = landingPad_width+landingPad_x;  //conenct the line on the landing pad
		p.y = landingPad_y;
		points.push_back(p);

    }
	
	 vector< XPoint > getTerrainPoints (){
		return points;
	}
	void paint(XInfo& xinfo) {
        // note the trick to pass a stl vector as an C array
        	XDrawLines(xinfo.display, xinfo.pixmap, xinfo.gc[1],
                   &points[0], points.size(),  // vector of points
                   CoordModeOrigin ); // use absolute coordinates

	}
	void reset(){
		points.clear();
	}
};
void drawMenuInstruction(string gameStatus, XInfo& xinfo);
class Message : public Displayable {
public:
	virtual void paint(XInfo& xinfo) {
		XDrawImageString( xinfo.display, xinfo.pixmap, xinfo.gc[1], x, y, this->s.c_str(), this->s.length() );
	}
	void paintMainScreen(XInfo& xinfo) {
		XDrawImageString( xinfo.display, xinfo.window, xinfo.gc[1], x, y, this->s.c_str(), this->s.length() );
	}
	void setString(string s){
		this->s=s;
	}
	Message (){
	}
	void setY(int y){
		this->y=y;
	}	
	void setX(int x){
		this->x=x;
	}
private:
	string s;
	int x;
	int y;
};

class Vision : public Displayable {
	XRectangle* visionRange;
	bool limitVision;
	public:
	Vision(){
		visionRange = new XRectangle();
		visionRange->x = 300;
    		visionRange->y = 20;
    		visionRange->width= maxVisionRangeWidth;
   		visionRange->height = maxVisionRangeHeight;	
		limitVision=true;	
	}
	~Vision(){
		delete visionRange;
	}
   	virtual void paint(XInfo& xinfo) {
		XFillRectangle(xinfo.display, xinfo.pixmap, xinfo.gc[1], //paint the screen black
			0, 0, xinfo.width, xinfo.height);
		XFillRectangle(xinfo.display, xinfo.pixmap, xinfo.gc[0], 
			visionRange->x, visionRange->y, visionRange->width, visionRange->height);
	}
	XRectangle* getVisionObject(){
		return visionRange;
	}
	void setX(int x){
		visionRange->x	=x;
	}
	void setY(int y){
		visionRange->y	=y;
	}
	int getVisionRange(){
		return visionRange->width;
	}
	void reset(){
		limitVision=false;
	}
};

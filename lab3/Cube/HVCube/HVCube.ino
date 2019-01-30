//Yueyang Cheng(1533989)
//Lab 3, Section B
/*
  Draws a 3d rotating cube on the ILI9341 screen. Each touch on screen will change
  the direction of rotation, horizontally or vertically. 
 */
#define CS_PIN  4
#define TFT_DC  9
#define TFT_CS 10

#include <SPI.h>
#include <ILI9341_t3.h>
#include <font_Arial.h> // from ILI9341_t3
#include <XPT2046_Touchscreen.h>

XPT2046_Touchscreen ts(CS_PIN);
#define TIRQ_PIN  2
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);

typedef struct
{
  uint16_t red   : 5;
  uint16_t green : 6;
  uint16_t blue  : 5;
} Color;


uint16_t Col;
Color PIX;


#define ScreenColor ILI9341_YELLOW
#define halfline 50

float angle;
float xx,xy,xz;
float yx,yy,yz;
float zx,zy,zz;

float fact;

int Xan,Yan;

int Xoff;
int Yoff;
int Zoff;

struct Point3d
{
  int x;
  int y;
  int z;
};

struct Point2d
{
  int x;
  int y;
};

int LinestoRender; // lines to render.
int OldLinestoRender; // lines to render just in case it changes. this makes sure the old lines all get erased.

struct Line3d
{
  Point3d p0;
  Point3d p1;
};

struct Line2d
{
  Point2d p0;
  Point2d p1;
};

Line3d Lines[20];
Line2d Render[20];
Line2d ORender[20];

/***********************************************************************************************************************************/
// Sets the global vars for the 3d transform. Any points sent through "process" will be transformed using these figures.
// only needs to be called if Xan or Yan are changed.
void SetVars(float val)
{
  float Xan2,Yan2,Zan2;
  float s1,s2,s3,c1,c2,c3;

  Xan2 = Xan / fact; // convert degrees to radians.
  Yan2 = Yan / fact;

  // Zan is assumed to be zero

  s1 = sin(Yan2);
  s2 = sin(Xan2);

  c1 = cos(Yan2);
  c2 = cos(Xan2);

  xx = c1 * val;
  xy = 0 * val;
  xz = -s1 * val;
  Serial.println(val);

  yx = (s1 * s2 * val);
  yy = c2 * val;
  yz = (c1 * s2 * val);

  zx = (s1 * c2 * val);
  zy = -s2 * val;
  zz = (c1 * c2 * val);
}
/***********************************************************************************************************************************/
// processes x1,y1,z1 and returns rx1,ry1 transformed by the variables set in SetVars()
// fairly heavy on floating point here.
// uses a bunch of global vars. Could be rewritten with a struct but possibly not worth the effort.
void ProcessLine(struct Line2d *ret,struct Line3d vec)
{
  float zvt1;
  int xv1,yv1,zv1;

  float zvt2;
  int xv2,yv2,zv2;

  int rx1,ry1;
  int rx2,ry2;

  int x1;
  int y1;
  int z1;

  int x2;
  int y2;
  int z2;

  int Ok;

  x1=vec.p0.x;
  y1=vec.p0.y;
  z1=vec.p0.z;

  x2=vec.p1.x;
  y2=vec.p1.y;
  z2=vec.p1.z;

  Ok=0; // defaults to not OK

  xv1 = (x1 * xx) + (y1 * xy) + (z1 * xz);
  yv1 = (x1 * yx) + (y1 * yy) + (z1 * yz);
  zv1 = (x1 * zx) + (y1 * zy) + (z1 * zz);

  zvt1 = zv1 - Zoff;


  if( zvt1 < -5){
    rx1 = 256 * (xv1 / zvt1) + Xoff;
    ry1 = 256 * (yv1 / zvt1) + Yoff;
    Ok=1; // ok we are alright for point 1.
  }

  xv2 = (x2 * xx) + (y2 * xy) + (z2 * xz);
  yv2 = (x2 * yx) + (y2 * yy) + (z2 * yz);
  zv2 = (x2 * zx) + (y2 * zy) + (z2 * zz);

  zvt2 = zv2 - Zoff;

  if( zvt2 < -5){
    rx2 = 256 * (xv2 / zvt2) + Xoff;
    ry2 = 256 * (yv2 / zvt2) + Yoff;
  } 
  else
  {
    Ok=0;
  }

  if(Ok==1){

    ret->p0.x=rx1;
    ret->p0.y=ry1;

    ret->p1.x=rx2;
    ret->p1.y=ry2;
  }
  // The ifs here are checks for out of bounds. needs a bit more code here to "safe" lines 
  //that will be way out of whack, so they dont get drawn and cause screen garbage.
}

/***********************************************************************************************************************************/
void setup() {

  tft.begin();
  Serial.begin(9600);

// in here is specific code for the display. change if you are using a different screen.
  tft.fillScreen(ScreenColor);

  fact = 180 / 3.14159259; // conversion from degrees to radians.

  Xoff = 120; // positions the center of the 3d conversion space into the center of the screen. This is usally screen_x_size / 2.
  Yoff = 160; // screen_y_size /2
  Zoff = 500;

  // line segments to draw a cube. basically p0 to p1. p1 to p2. p2 to p3 so on.
  // Front Face.

  Lines[0].p0.x=-halfline;
  Lines[0].p0.y=-halfline;
  Lines[0].p0.z=halfline;
  Lines[0].p1.x=halfline;
  Lines[0].p1.y=-halfline;
  Lines[0].p1.z=halfline;

  Lines[1].p0.x=halfline;
  Lines[1].p0.y=-halfline;
  Lines[1].p0.z=halfline;
  Lines[1].p1.x=halfline;
  Lines[1].p1.y=halfline;
  Lines[1].p1.z=halfline;

  Lines[2].p0.x=halfline;
  Lines[2].p0.y=halfline;
  Lines[2].p0.z=halfline;
  Lines[2].p1.x=-halfline;
  Lines[2].p1.y=halfline;
  Lines[2].p1.z=halfline;

  Lines[3].p0.x=-halfline;
  Lines[3].p0.y=halfline;
  Lines[3].p0.z=halfline;
  Lines[3].p1.x=-halfline;
  Lines[3].p1.y=-halfline;
  Lines[3].p1.z=halfline;

  //back face.

  Lines[4].p0.x=-halfline;
  Lines[4].p0.y=-halfline;
  Lines[4].p0.z=-halfline;
  Lines[4].p1.x=halfline;
  Lines[4].p1.y=-halfline;
  Lines[4].p1.z=-halfline;

  Lines[5].p0.x=halfline;
  Lines[5].p0.y=-halfline;
  Lines[5].p0.z=-halfline;
  Lines[5].p1.x=halfline;
  Lines[5].p1.y=halfline;
  Lines[5].p1.z=-halfline;

  Lines[6].p0.x=halfline;
  Lines[6].p0.y=halfline;
  Lines[6].p0.z=-halfline;
  Lines[6].p1.x=-halfline;
  Lines[6].p1.y=halfline;
  Lines[6].p1.z=-halfline;

  Lines[7].p0.x=-halfline;
  Lines[7].p0.y=halfline;
  Lines[7].p0.z=-halfline;
  Lines[7].p1.x=-halfline;
  Lines[7].p1.y=-halfline;
  Lines[7].p1.z=-halfline;

  // now the 4 edge lines.

  Lines[8].p0.x=-halfline;
  Lines[8].p0.y=-halfline;
  Lines[8].p0.z=halfline;
  Lines[8].p1.x=-halfline;
  Lines[8].p1.y=-halfline;
  Lines[8].p1.z=-halfline;

  Lines[9].p0.x=halfline;
  Lines[9].p0.y=-halfline;
  Lines[9].p0.z=halfline;
  Lines[9].p1.x=halfline;
  Lines[9].p1.y=-halfline;
  Lines[9].p1.z=-halfline;

  Lines[10].p0.x=-halfline;
  Lines[10].p0.y=halfline;
  Lines[10].p0.z=halfline;
  Lines[10].p1.x=-halfline;
  Lines[10].p1.y=halfline;
  Lines[10].p1.z=-halfline;

  Lines[11].p0.x=halfline;
  Lines[11].p0.y=halfline;
  Lines[11].p0.z=halfline;
  Lines[11].p1.x=halfline;
  Lines[11].p1.y=halfline;
  Lines[11].p1.z=-halfline;

  LinestoRender=12;
  OldLinestoRender=LinestoRender;
}
/***********************************************************************************************************************************/


void RenderImage( void)
{
  // renders all the lines after erasing the old ones.
  // in here is the only code actually interfacing with the screen. so if you use a different lib, this is where to change it.

  for (int i=0; i<OldLinestoRender; i++ )
  {
    tft.drawLine(ORender[i].p0.x,ORender[i].p0.y,ORender[i].p1.x,ORender[i].p1.y, ScreenColor); // erase the old lines.
  }

  for (int i=0; i<LinestoRender; i++ )
  {
    tft.drawLine(Render[i].p0.x,Render[i].p0.y,Render[i].p1.x,Render[i].p1.y, *(uint16_t*)&PIX);
  }
  OldLinestoRender=LinestoRender;
}

// Translate a hue "angle" -120 to 120 degrees (ie -2PI/3 to 2PI/3) to
// a 6-bit R channel value
//
inline int angle_to_channel(float a) {
  if(a < -PI)
    a += 2*PI;
  if(a < -2*PI/3  || a > 2*PI/3)
    return 0;
  float f_channel = cos(a*3/4); // remap 120-degree 0-1.0 to 90 ??
  return ceil(f_channel * 63);
}

/***********************************************************************************************************************************/

volatile boolean moving = false;
volatile boolean wastouched = false;
volatile int counter;
void loop() {
  float val = analogRead(2) / 100.0; // zooming factor
  PIX.red = angle_to_channel(angle)>>1;
  PIX.green = angle_to_channel(angle-2*PI/3);
  PIX.blue = angle_to_channel(angle-4*PI/3)>>1;

  angle += 0.02;
  if(angle > PI)
    angle -= 2*PI;
    
  boolean istouched = ts.touched();

  // in case the touch screen is too sensitive
  if(istouched && wastouched) {
    counter++;
  } else {
    counter = 0;
  }
  if(counter == 3) {
    moving = !moving; // change state
  }
  
  if(moving){
    Xan++;
  }else {
    Yan++;
  } 
  wastouched = istouched; // update wastouched value

  Yan=Yan % 360;
  Xan=Xan % 360; // prevents overflow.
  Serial.println(counter);
  SetVars(val); //sets up the global vars to do the conversion.

  for(int i=0; i<LinestoRender ; i++)
  {
    ORender[i]=Render[i]; // stores the old line segment so we can delete it later.
    ProcessLine(&Render[i],Lines[i]); // converts the 3d line segments to 2d.
  } 

  RenderImage(); // go draw it!
  delay(40);
}
/***********************************************************************************************************************************/



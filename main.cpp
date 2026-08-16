#include <windows.h>
#include <mmsystem.h>
#include <GL/glut.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#pragma comment(lib, "winmm.lib")
using namespace std;

// ── state flags ───────────────────────────────────────────────────────────────
bool snow         = false;
bool snowstorm    = false;
bool day          = true;
bool afternoon    = false;
bool night        = false;
bool transitioning= false;
float snowIntensity  = 0.0f;
float afternoonBlend = 0.0f;

// ── sound flags ───────────────────────────────────────────────────────────────
bool snowstormSoundPlaying = false;
bool snowSoundPlaying      = false;
bool nightAmbientPlaying   = false;

void stopAllSounds()
{
    PlaySound(NULL, NULL, 0);
}

void playSnowAmbient()
{
    if (!snowSoundPlaying)
    {
        PlaySound(TEXT("snow.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
        snowSoundPlaying      = true;
        snowstormSoundPlaying = false;
        nightAmbientPlaying   = false;
    }
}

void playStormAmbient()
{
    if (!snowstormSoundPlaying)
    {
        PlaySound(TEXT("storm.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
        snowstormSoundPlaying = true;
        snowSoundPlaying      = false;
        nightAmbientPlaying   = false;
    }
}

void playNightAmbient()
{
    if (!nightAmbientPlaying && !snow && !snowstorm)
    {
        PlaySound(TEXT("night.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
        nightAmbientPlaying   = true;
        snowSoundPlaying      = false;
        snowstormSoundPlaying = false;
    }
}

void updateAmbientSound()
{
    if (snowstorm)
    {
        if (!snowstormSoundPlaying)   // only call PlaySound once
        {
            PlaySound(TEXT("storm.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
            snowstormSoundPlaying = true;
            snowSoundPlaying      = false;
            nightAmbientPlaying   = false;
        }
    }
    else if (snow)
    {
        if (!snowSoundPlaying)
        {
            PlaySound(TEXT("snow.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
            snowSoundPlaying      = true;
            snowstormSoundPlaying = false;
            nightAmbientPlaying   = false;
        }
    }
    else if (night)
    {
        if (!nightAmbientPlaying)
        {
            PlaySound(TEXT("night.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
            nightAmbientPlaying   = true;
            snowSoundPlaying      = false;
            snowstormSoundPlaying = false;
        }
    }
    else
    {
        stopAllSounds();
        snowSoundPlaying      = false;
        snowstormSoundPlaying = false;
        nightAmbientPlaying   = false;
    }
}

// ── primitive draw helpers ────────────────────────────────────────────────────
void drawCircle(float cx, float cy, float r)
{
    glBegin(GL_POLYGON);
    for (int i = 0; i < 200; ++i)
    {
        float theta = 2.0f * 3.1415926f * i / 200.0f;
        glVertex2f(cx + r * cosf(theta), cy + r * sinf(theta));
    }
    glEnd();
}
void drawLines(float x1, float y1, float x2, float y2)
{
    glBegin(GL_LINES); glVertex2f(x1,y1); glVertex2f(x2,y2); glEnd();
}
void drawTriangle(float x1,float y1,float x2,float y2,float x3,float y3)
{
    glBegin(GL_TRIANGLES); glVertex2f(x1,y1); glVertex2f(x2,y2); glVertex2f(x3,y3); glEnd();
}
void drawRectangle(float x1,float y1,float x2,float y2,float x3,float y3,float x4,float y4)
{
    glBegin(GL_POLYGON); glVertex2f(x1,y1); glVertex2f(x2,y2); glVertex2f(x3,y3); glVertex2f(x4,y4); glEnd();
}
void drawPentagon(float x1,float y1,float x2,float y2,float x3,float y3,float x4,float y4,float x5,float y5)
{
    glBegin(GL_POLYGON);
    glVertex2f(x1,y1); glVertex2f(x2,y2); glVertex2f(x3,y3); glVertex2f(x4,y4); glVertex2f(x5,y5);
    glEnd();
}
void drawQuad(float x1,float y1,float x2,float y2,float x3,float y3,float x4,float y4)
{
    glBegin(GL_QUADS); glVertex2f(x1,y1); glVertex2f(x2,y2); glVertex2f(x3,y3); glVertex2f(x4,y4); glEnd();
}

// ── global time ───────────────────────────────────────────────────────────────
float globalTime = 0.0f;

// ════════════════════════════════════════════════════════════════════════════
//  SKY
// ════════════════════════════════════════════════════════════════════════════
float sunMoon        = 0.0f;
float skyTransition  = 0.0f;
bool  goingToNight   = false;

void drawSunMoon()
{
    glPushMatrix();
    glTranslatef(sunMoon, 0, 0);
    float sunY    = afternoon ? 48.0f : 60.0f;
    float sunSize = afternoon ? 5.0f  : 4.0f;
    if (afternoon)  glColor3ub(255, 160, 40);
    else if (day)   glColor3ub(255, 215, 0);
    else if (night) glColor3ub(230, 230, 220);
    drawCircle(15, sunY, sunSize);
    if ((day || afternoon) && snowIntensity < 0.5f)
    {
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        if (afternoon)
        {
            glColor4f(1.0f, 0.55f, 0.1f, 0.20f); drawCircle(15, sunY, 8.5f);
            glColor4f(1.0f, 0.40f, 0.0f, 0.08f); drawCircle(15, sunY, 13.0f);
        }
        else
        {
            glColor4f(1.0f, 0.9f, 0.3f, 0.15f); drawCircle(15, sunY, 6.5f);
        }
        glDisable(GL_BLEND);
    }
    glPopMatrix();
}

void updateSunMoon(int value)
{
    if (transitioning)
    {
        sunMoon += 10;
        skyTransition += goingToNight ? 0.08f : -0.08f;
        if (skyTransition < 0) skyTransition = 0;
        if (skyTransition > 1) skyTransition = 1;
        if (sunMoon > 100) { sunMoon = 0; transitioning = false; }
    }
    glutPostRedisplay();
    glutTimerFunc(100, updateSunMoon, 0);
}

#define NUM_STARS 70
float starX[NUM_STARS], starY[NUM_STARS];
bool starsInitialized = false;
void initStars()
{
    for (int i = 0; i < NUM_STARS; i++)
    { starX[i] = rand() % 100; starY[i] = 30 + rand() % 37; }
    starsInitialized = true;
}
void drawStars()
{
    if (!night) return;
    if (!starsInitialized) initStars();
    for (int i = 0; i < NUM_STARS; i++)
    {
        float bright = 180 + 75 * sinf(globalTime * 2.0f + i * 0.7f);
        glColor3ub((GLubyte)bright, (GLubyte)bright, (GLubyte)bright);
        glPointSize(1.5f);
        glBegin(GL_POINTS); glVertex2f(starX[i], starY[i]); glEnd();
    }
}

void skyGradient(float t)
{
    unsigned char topR, topG, topB, botR, botG, botB;

    if (afternoon && !transitioning)
    {
        float b = afternoonBlend;
        topR = (unsigned char)(135 + b * (100 - 135));
        topG = (unsigned char)(206 + b * (160 - 206));
        topB = (unsigned char)(250 + b * (180 - 250));
        botR = (unsigned char)(84  + b * (255 - 84));
        botG = (unsigned char)(155 + b * (140 - 155));
        botB = (unsigned char)(235 + b * (80  - 235));
    }
    else if (t < 0.5f)
    {
        float p = t * 2.0f;
        topR = (unsigned char)(135 + p * (80 - 135));
        topG = (unsigned char)(206 + p * (80 - 206));
        topB = (unsigned char)(250 + p * (60 - 250));
        botR = (unsigned char)(84  + p * (200 - 84));
        botG = (unsigned char)(155 + p * (100 - 155));
        botB = (unsigned char)(235 + p * (60 - 235));
    }
    else
    {
        float p = (t - 0.5f) * 2.0f;
        topR = (unsigned char)(80  + p * (0  - 80));
        topG = (unsigned char)(80  + p * (0  - 80));
        topB = (unsigned char)(60  + p * (76 - 60));
        botR = (unsigned char)(200 + p * (0  - 200));
        botG = (unsigned char)(100 + p * (0  - 100));
        botB = (unsigned char)(60  + p * (25 - 60));
    }
    glBegin(GL_POLYGON);
    glColor3ub(topR, topG, topB); glVertex2f(0,67); glVertex2f(100,67);
    glColor3ub(botR, botG, botB); glVertex2f(100,26); glVertex2f(0,26);
    glEnd();
}

void drawAfternoonOverlay()
{
    if (afternoonBlend <= 0.0f) return;
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 0.55f, 0.05f, 0.13f * afternoonBlend);
    glBegin(GL_POLYGON);
    glVertex2f(0,0); glVertex2f(100,0); glVertex2f(100,30); glVertex2f(0,30);
    glEnd();
    glColor4f(0.0f, 0.0f, 0.0f, 0.10f * afternoonBlend);
    for (int i = 0; i < 6; i++)
    {
        float bx = 10.0f + i * 15.0f;
        glBegin(GL_POLYGON);
        glVertex2f(bx,     29.0f);
        glVertex2f(bx+2.5f,29.0f);
        glVertex2f(bx+18.0f,9.0f);
        glVertex2f(bx+15.5f,9.0f);
        glEnd();
    }
    glDisable(GL_BLEND);
}

float cloudOffsets[5] = { 10, 30, 50, 70, 90.0f };
void drawCloudAt(float ox, float oy)
{
    glTranslatef(ox, oy, 0);
    if (afternoon && afternoonBlend > 0.3f) glColor3ub(255, 210, 160);
    else if (day && skyTransition < 0.4f)   glColor3ub(255, 255, 255);
    else if (snowstorm)                     glColor3ub(50, 50, 60);
    else if (night)                         glColor3ub(90, 90, 105);
    else                                    glColor3ub(230, 180, 130);
}
void drawCloud1() { glPushMatrix(); drawCloudAt(cloudOffsets[0],60); drawCircle(0,0,4);drawCircle(-4,1.5,3);drawCircle(4,1.5,3);drawCircle(-2.5,-1.5,3);drawCircle(2.5,-1.5,3);drawCircle(0,3,2.5); glPopMatrix(); }
void drawCloud2() { glPushMatrix(); drawCloudAt(cloudOffsets[1],62); drawCircle(0,0,3.5);drawCircle(-3.5,1.5,2.5);drawCircle(3.5,1.5,2.5);drawCircle(-2.5,-1.5,2.5);drawCircle(2.5,-1.5,2.5);drawCircle(0,2.5,1.8); glPopMatrix(); }
void drawCloud3() { glPushMatrix(); drawCloudAt(cloudOffsets[2],58); drawCircle(0,0,3);drawCircle(-3,1,2);drawCircle(3,1,2);drawCircle(-1.5,-1,2);drawCircle(1.5,-1,2);drawCircle(0,2.5,1.5); glPopMatrix(); }
void drawCloud4() { glPushMatrix(); drawCloudAt(cloudOffsets[3],55); drawCircle(-3,-1.5,2.5);drawCircle(0,-1.8,3);drawCircle(3,-1.5,2.5);drawCircle(-2,1.2,2);drawCircle(0,1.5,2.2);drawCircle(2,1.2,2);drawCircle(0,3.5,1.5); glPopMatrix(); }
void drawCloud5() { glPushMatrix(); drawCloudAt(cloudOffsets[4],54); drawCircle(0,0,2.5);drawCircle(-4,0.5,2);drawCircle(4,0.5,2);drawCircle(-6,0,1.5);drawCircle(6,0,1.5);drawCircle(0,2,1.2); glPopMatrix(); }

void updateClouds(int value)
{
    float spd = snowstorm ? 1.2f : 0.5f;
    for (int i = 0; i < 5; ++i)
    { cloudOffsets[i] -= spd; if (cloudOffsets[i] < -6) cloudOffsets[i] = 100.0f; }
    glutPostRedisplay();
    glutTimerFunc(100, updateClouds, 0);
}

void sky()
{
    float t = transitioning ? skyTransition : (night ? 1.0f : 0.0f);
    skyGradient(t);
    drawSunMoon();
    drawCloud1(); drawCloud2(); drawCloud3(); drawCloud4(); drawCloud5();
    drawStars();
}

void updateAfternoon(int value)
{
    if (afternoon) { afternoonBlend += 0.04f; if (afternoonBlend > 1.0f) afternoonBlend = 1.0f; }
    else           { afternoonBlend -= 0.04f; if (afternoonBlend < 0.0f) afternoonBlend = 0.0f; }
    glutPostRedisplay();
    glutTimerFunc(40, updateAfternoon, 0);
}

// ── hills ─────────────────────────────────────────────────────────────────────
void drawHill()
{
    glPushMatrix(); glTranslatef(0,5,0);
    if(day) glColor3ub(51,109,129); else glColor3ub(20,45,70);
    drawTriangle(6.5,24,20.5,24,21,43);
    if(day) glColor3ub(40,92,114); else glColor3ub(10,35,55);
    drawTriangle(20.5,24,21,43,36,24);
    if(day) glColor3ub(250,233,217); else glColor3ub(100,100,120);
    glBegin(GL_POLYGON); glVertex2f(16,37);glVertex2f(17.5,37);glVertex2f(18,36);glVertex2f(19,37);glVertex2f(20,37);glVertex2f(21,38);glVertex2f(21,43); glEnd();
    if(day) glColor3ub(232,230,217); else glColor3ub(80,80,100);
    glBegin(GL_POLYGON); glVertex2f(21,38);glVertex2f(22,38.5);glVertex2f(23,38);glVertex2f(23.5,36);glVertex2f(24,38);glVertex2f(25,37);glVertex2f(25.3,38);glVertex2f(21,43); glEnd();
    glPopMatrix();
}
void drawHll3()
{
    glPushMatrix(); glTranslatef(40,0,0); drawHill(); glPopMatrix();
    glPushMatrix(); glTranslatef(60,3,0); drawHill(); glPopMatrix();
}
void drawHill2()
{
    glPushMatrix(); glTranslatef(0,5,0);
    if(day) glColor3ub(51,109,129); else glColor3ub(20,45,70);
    drawTriangle(25,38,34,26,34,49);
    if(day) glColor3ub(40,92,114); else glColor3ub(10,35,55);
    drawTriangle(34,26,34,49,54,26);
    if(day) glColor3ub(250,233,217); else glColor3ub(100,100,120);
    glBegin(GL_POLYGON); glVertex2f(28,42);glVertex2f(29,43);glVertex2f(30,40);glVertex2f(32,41.4);glVertex2f(33,43);glVertex2f(34,40);glVertex2f(34,49); glEnd();
    if(day) glColor3ub(232,230,217); else glColor3ub(80,80,100);
    glBegin(GL_POLYGON); glVertex2f(34,42);glVertex2f(35,43);glVertex2f(37,40);glVertex2f(38,42);glVertex2f(39,43);glVertex2f(40,42);glVertex2f(34,49); glEnd();
    glPopMatrix();
}

// ── road ──────────────────────────────────────────────────────────────────────
void Road()
{
    if(day) glColor3ub(0,0,0); else glColor3ub(30,30,30);
    drawRectangle(0,9.0,0,8.6,100,8.6,100,9.0);
    if(day) glColor3ub(50,85,97); else glColor3ub(25,50,60);
    drawRectangle(100,8.5,100,8.2,0,8.2,0,8.5);
    if(day) glColor3ub(50,85,97); else glColor3ub(20,40,50);
    drawRectangle(100,8.1,100,4.2,0,4.2,0,8.1);
    if(day) glColor3ub(0,0,0); else glColor3ub(30,30,30);
    drawRectangle(100,4.2,100,3.95,0,3.95,0,4.2);
    if(day) glColor3ub(84,117,121); else glColor3ub(40,60,70);
    drawRectangle(100,3.95,100,3.6,0,3.6,0,3.95);
    if(day) glColor3ub(0,0,0); else glColor3ub(30,30,30);
    drawRectangle(100,3.6,100,3.4,0,3.4,0,3.6);
    if(day) glColor3ub(255,255,255); else glColor3ub(200,200,200);
    for (float x = 5; x < 100; x += 10) drawRectangle(x,6.3,x+3,6.3,x+3,6.0,x,6.0);
}

// ── snowman ───────────────────────────────────────────────────────────────────
void drawSnowman(float scale)
{
    glPushMatrix(); glTranslatef(15,-2,0);
    if(day) glColor3f(1,1,1); else glColor3f(0.85f,0.85f,0.9f);
    glBegin(GL_POLYGON); for(int i=0;i<360;i++){float t=i*3.14159f/180;glVertex2f(75+3.5f*cosf(t),36+2.5f*sinf(t));} glEnd();
    glBegin(GL_POLYGON); for(int i=0;i<360;i++){float t=i*3.14159f/180;glVertex2f(75+2.5f*cosf(t),39+2*sinf(t));} glEnd();
    glColor3f(0,0,0);
    glBegin(GL_POLYGON); glVertex2f(72,40);glVertex2f(78,40);glVertex2f(77,40.5);glVertex2f(73,40.5); glEnd();
    glBegin(GL_POLYGON); glVertex2f(73.5,40.5);glVertex2f(76.5,40.5);glVertex2f(76.5,42);glVertex2f(73.5,42); glEnd();
    if(day) glColor3f(1,0,0); else glColor3f(0.6f,0,0);
    glBegin(GL_POLYGON); glVertex2f(72.5,37.5);glVertex2f(77.5,37.5);glVertex2f(77,38);glVertex2f(73,38); glEnd();
    glBegin(GL_POLYGON); glVertex2f(73,37.5);glVertex2f(74,37.5);glVertex2f(74,36);glVertex2f(72.5,36.5); glEnd();
    glBegin(GL_POLYGON); glVertex2f(76,37.5);glVertex2f(77,37.5);glVertex2f(77.5,36.5);glVertex2f(76,36); glEnd();
    glColor3f(0.55f,0.27f,0.07f); glLineWidth(2);
    glBegin(GL_LINES); glVertex2f(71.5,36);glVertex2f(73.5,37); glEnd();
    glBegin(GL_LINES); glVertex2f(76.5,37);glVertex2f(78.5,36); glEnd();
    glColor3f(0,0,0); glPointSize(3);
    glBegin(GL_POINTS); glVertex2f(74,39.5);glVertex2f(76,39.5); glEnd();
    glBegin(GL_LINES); glVertex2f(74,38.5);glVertex2f(76,38.5); glEnd();
    glPointSize(2);
    glBegin(GL_POINTS); glVertex2f(75,37);glVertex2f(75,36);glVertex2f(75,35); glEnd();
    glPopMatrix();
}

// ── snow ──────────────────────────────────────────────────────────────────────
struct Snowflake { float x, y, drift, speed; };
std::vector<Snowflake> snowflakes;

void initSnowflakes()
{
    snowflakes.clear();
    for (int i = 0; i < 200; ++i)
        snowflakes.push_back({ (float)(rand()%100), (float)(rand()%67),
                                (float)(rand()%100) * 0.01f - 0.5f,
                                0.3f + (rand()%10)*0.05f });
}
void drawSnow()
{
    if(day) glColor3f(1,1,1); else glColor3f(0.85f,0.9f,0.95f);
    for (auto& f : snowflakes) drawCircle(f.x, f.y, 0.25f);
}
void updateSnow(int value)
{
    if (snow || snowstorm)
    {
        for (auto& f : snowflakes)
        {
            float vY = snowstorm ? 2.5f : f.speed;
            float vX = snowstorm ? (sinf(globalTime + f.y * 0.1f) * 1.5f)
                                 : (sinf(globalTime * 0.5f + f.y * 0.2f) * 0.3f + f.drift * 0.1f);
            f.y -= vY;
            f.x += vX;
            if (f.y < 0)  { f.y = 67.0f; f.x = (float)(rand()%100); }
            if (f.x < 0)  f.x = 100.0f;
            if (f.x > 100) f.x = 0.0f;
        }
        glutPostRedisplay();
        glutTimerFunc(50, updateSnow, 0);
    }
}
void updateSnowIntensity(int value)
{
    if (snow||snowstorm) { snowIntensity+=0.01f; if(snowIntensity>1)snowIntensity=1; }
    else                 { if(snowIntensity>0) snowIntensity-=0.01f; }
    glutPostRedisplay();
    glutTimerFunc(100, updateSnowIntensity, 0);
}

// ── lightning ─────────────────────────────────────────────────────────────────
struct LightningBranch { float x1,y1,x2,y2; };
std::vector<LightningBranch> lightningBranches;
bool showLightning=false;
int  lightningDuration=0;
float lightningFlashAlpha=0.0f;

void generateLightning(float startX, float startY)
{
    lightningBranches.clear();
    float x=startX, y=startY;
    while(y>28.75f)
    {
        float nx=x+(rand()%20-10); if(nx<0)nx=0; if(nx>100)nx=100;
        float ny=y-(rand()%8+5);   if(ny<28.75f)ny=28.75f;
        lightningBranches.push_back({x,y,nx,ny});
        x=nx; y=ny;
        if(rand()%100<35)
        {
            float bx=x+(rand()%16-8); if(bx<0)bx=0; if(bx>100)bx=100;
            float by=y-(rand()%5+3); if(by<28.75f)by=28.75f;
            lightningBranches.push_back({x,y,bx,by});
        }
    }
}
void drawLightningFlash()
{
    if(!showLightning) return;
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1,1,1,lightningFlashAlpha*0.15f);
    drawRectangle(0,26,100,26,100,67,0,67);
    glDisable(GL_BLEND);
    glColor3f(0.9f,0.95f,1.0f); glLineWidth(2.5f);
    glBegin(GL_LINES);
    for(auto& b:lightningBranches){glVertex2f(b.x1,b.y1);glVertex2f(b.x2,b.y2);}
    glEnd();
    glLineWidth(1.0f);
}
void updateLightningFlash(int value)
{
    if(snowstorm)
    {
        if(lightningDuration>0) { lightningDuration--; lightningFlashAlpha=lightningDuration*0.3f; }
        else
        {
            if(rand()%100<25)
            {
                generateLightning((float)(rand()%100),67.0f);
                lightningDuration=4;
                showLightning=true;
                lightningFlashAlpha=1.0f;
                // SND_NOSTOP means: play thunder only if nothing else is playing on this channel
                // But since we need it alongside storm, use a separate approach:
                PlaySound(TEXT("thunder.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_NOSTOP);
            }
            else showLightning=false;
        }
    }
    else showLightning=false;
    glutPostRedisplay();
    glutTimerFunc(150, updateLightningFlash, 0);
}
// ── windmill ──────────────────────────────────────────────────────────────────
float windmillAngle=0;
void drawWindmill()
{
    glPushMatrix(); glTranslatef(0,2,0);
    if(day) glColor3ub(200,200,200); else glColor3ub(120,120,130);
    glBegin(GL_QUADS);
    glVertex2f(80.36,28);glVertex2f(82,28);glVertex2f(81.64,40.46);glVertex2f(81,40);
    glEnd();
    if(day) glColor3ub(100,100,100); else glColor3ub(70,70,80);
    drawCircle(81.32,41,0.8f);
    glPushMatrix(); glTranslatef(81.32,41,0); glRotatef(windmillAngle,0,0,1);
    if(day) glColor3ub(180,200,220); else glColor3ub(100,120,140);
    for(int i=0;i<4;i++){drawPentagon(0.49,0.73,1.96,1.48,7.06,1.66,2.415,0.13,0.63,0.13); glRotatef(90,0,0,1);}
    glPopMatrix(); glPopMatrix();
}
void windmillUpdate(int value)
{
    float spd = snowstorm ? 4.0f : (snow ? 2.5f : 2.0f);
    windmillAngle += spd; if(windmillAngle>360) windmillAngle-=360;
    glutPostRedisplay(); glutTimerFunc(16, windmillUpdate, 0);
}

// ── boat ──────────────────────────────────────────────────────────────────────
float boatX   = -11;
float boatRock = 0.0f;

void drawBoat()
{
    glPushMatrix();
    glTranslatef(boatX, 0, 0);
    float cx=89.0f, cy=20.0f;
    glTranslatef(cx,cy,0);
    glRotatef(3.0f * sinf(boatRock), 0,0,1);
    glTranslatef(-cx,-cy,0);

    glColor3ub(111,78,55);
    glPushMatrix(); glTranslatef(4,5,0);
    drawRectangle(79,16,91,16,90,14,80,14);
    glColor3ub(255,228,181);
    drawTriangle(85.2,23,82,17,88,17);
    glColor3ub(111,78,55);
    drawRectangle(84.7,14,85.7,14,85.7,23,84.7,23);
    if(day) glColor3ub(160,220,255); else glColor3ub(60,100,150);
    glBegin(GL_TRIANGLES);
    glVertex2f(79,14.5); glVertex2f(70,15.5); glVertex2f(70,13.5);
    glEnd();
    glPopMatrix(); glPopMatrix();
}
void updateBoat(int value)
{
    boatX   += 0.3f; if(boatX>22) boatX=-10;
    boatRock += 0.08f;
    glutPostRedisplay(); glutTimerFunc(100, updateBoat, 0);
}

// ── lake ──────────────────────────────────────────────────────────────────────
float lakeX=1.0f, lakeY=1.4f;
bool waterLabel1=false, waterLabel2=false;
float waveOffset=0.0f;

void lakeShape()
{
    glBegin(GL_POLYGON);
    if(day) glColor3ub(10,60,120); else glColor3ub(5,25,55);
    glVertex2f(101,12); glVertex2f(95.2,12.5); glVertex2f(90.78,12.712); glVertex2f(84.027,11.55); glVertex2f(79.25,12.25);
    if(day) glColor3ub(30,110,190); else glColor3ub(10,50,95);
    glVertex2f(77.13,14.23); glVertex2f(75.71,16.5); glVertex2f(74.63,17.24); glVertex2f(73.88,17.88); glVertex2f(72.88,18.5);
    if(day) glColor3ub(80,170,230); else glColor3ub(30,80,130);
    glVertex2f(72.05,19.38); glVertex2f(71.64,20.25); glVertex2f(71.67,21.13); glVertex2f(72,25);
    glVertex2f(72.53,25.74); glVertex2f(73.41,26.35); glVertex2f(74.48,26.8); glVertex2f(75.86,27.21);
    glVertex2f(77.46,27.39); glVertex2f(79.87,28.4); glVertex2f(83.54,28.96); glVertex2f(87.23,29);
    glVertex2f(91.27,29.78); glVertex2f(100,30.09);
    glEnd();
}

void drawLake(float r1, float g1, float b1)
{
    glColor3ub(62,49,23); lakeShape();
    glColor3ub(r1,g1,b1);
    glPushMatrix();
    if(waterLabel1) glScalef(lakeX,1,1);
    if(waterLabel2) glScalef(1,lakeY,1);
    lakeShape();
    glPopMatrix();

    if(snowIntensity < 0.85f)
    {
        glLineWidth(0.8f);
        for(int i=0;i<5;i++)
        {
            float base = 74.0f + i * 5.5f;
            float slide= base + fmodf(waveOffset * 3.0f + i * 7.0f, 18.0f);
            float y    = 22.0f + i * 1.5f;
            float alpha= 0.5f + 0.5f * sinf(waveOffset * 2.0f + i);
            if(day) glColor3f(alpha,alpha,alpha); else glColor3f(0.4f*alpha,0.6f*alpha,0.85f*alpha);
            glBegin(GL_LINES); glVertex2f(slide,y); glVertex2f(slide+3.0f,y+0.25f); glEnd();
        }
        glLineWidth(1.0f);
    }

    if(snowIntensity > 0.1f)
    {
        if(day) glColor3f(0.85f,0.95f,1.0f); else glColor3f(0.55f,0.70f,0.82f);
        glBegin(GL_POLYGON);
        glVertex2f(72,25.5f); glVertex2f(73.41,26.35f); glVertex2f(75.86,27.21f);
        glVertex2f(79.87,28.4f); glVertex2f(83.54,28.96f); glVertex2f(91.27,29.78f);
        glVertex2f(100,30.09f); glVertex2f(101,28.0f); glVertex2f(90.0f,27.5f);
        glVertex2f(80.0f,26.8f); glVertex2f(75.0f,25.8f);
        glEnd();
        if(snowIntensity > 0.6f)
        {
            if(day) glColor3f(0.65f,0.82f,0.95f); else glColor3f(0.30f,0.50f,0.68f);
            glLineWidth(0.6f);
            glBegin(GL_LINES);
            glVertex2f(78,27.5f); glVertex2f(82,28.2f); glVertex2f(82,28.2f); glVertex2f(85,27.8f);
            glVertex2f(85,27.8f); glVertex2f(89,28.6f); glVertex2f(80,28.0f); glVertex2f(80,26.5f);
            glVertex2f(86,29.0f); glVertex2f(87,27.2f);
            glEnd();
            glLineWidth(1.0f);
        }
    }
}

void updateLake(int value)
{
    waveOffset += 0.04f;
    if(waveOffset>100.0f) waveOffset=0.0f;
    if(waterLabel1){ lakeX+=0.01f; if(lakeX>1.1f)lakeX=1.1f; }
    if(waterLabel2){ lakeY-=0.01f; if(lakeY<1.0f)lakeY=1.0f; }
    glutPostRedisplay();
    glutTimerFunc(40, updateLake, 0);
}

// ── backside / ground ─────────────────────────────────────────────────────────
void backside()
{
    auto C=[](){ if(day) glColor3ub(60,124,134); else glColor3ub(30,65,70); };
    C(); glBegin(GL_POLYGON); glVertex2f(60,30);glVertex2f(60,41);glVertex2f(70,47);glVertex2f(80,35);glVertex2f(90,47);glVertex2f(95,41);glVertex2f(100,45);glVertex2f(100,30); glEnd();
    C(); glBegin(GL_POLYGON); glVertex2f(0,20);glVertex2f(0,36);glVertex2f(3,38);glVertex2f(7,43);glVertex2f(10,41);glVertex2f(20,37); glEnd();
    C(); glBegin(GL_POLYGON); glVertex2f(40,35);glVertex2f(42,45);glVertex2f(45,50);glVertex2f(50,45);glVertex2f(55,35);glVertex2f(55,50);glVertex2f(60,45); glEnd();
}
void drawGound()
{
    glPushMatrix(); glTranslatef(0,5,0);
    glBegin(GL_POLYGON);
    glVertex2f(0,4);glVertex2f(0,24);glVertex2f(9.63,29.24);glVertex2f(11.85,28.56);
    glVertex2f(14.07,28.17);glVertex2f(16.97,27.11);glVertex2f(19.48,25.85);
    glVertex2f(22.66,25.08);glVertex2f(26.82,24.70);glVertex2f(31.68,24.41);
    glVertex2f(37.88,23.85);glVertex2f(53.19,23.86);glVertex2f(57.85,24.45);
    glVertex2f(61,25);glVertex2f(62.81,26.05);glVertex2f(65.64,26.59);
    glVertex2f(67.89,27.31);glVertex2f(73.61,28.05);glVertex2f(82,28);
    glVertex2f(91.88,28.5);glVertex2f(96.12,28.25);glVertex2f(100,30);glVertex2f(100,4);
    glEnd();
    glPopMatrix();
    glBegin(GL_POLYGON); glVertex2f(0,0);glVertex2f(0,3.3);glVertex2f(100,3.3);glVertex2f(100,0); glEnd();
}

// ── trees ─────────────────────────────────────────────────────────────────────
float treeAngle=0.0f;

void drawTree1()
{
    if(day) glColor3ub(101,67,33); else glColor3ub(60,40,20);
    drawRectangle(6.5,26,7.5,26,7.5,30,6.5,30);
    glPushMatrix();
    glTranslatef(7.0,30.0,0);
    float sway = snowstorm ? 5.0f*sinf(treeAngle) : (snow ? 2.0f*sinf(treeAngle*0.7f) : 0);
    glRotatef(sway,0,0,1);
    glTranslatef(-7.0,-30.0,0);
    if(day) glColor3ub(46,138,87); else glColor3ub(20,60,40);
    drawCircle(5.5,31,sqrtf(4.0f)); drawCircle(8.5,31,sqrtf(4.0f)); drawCircle(7,32.5,sqrtf(5.0f));
    if(day) glColor3ub(60,179,113); else glColor3ub(35,90,60);
    drawCircle(6,35,sqrtf(6.0f)); drawCircle(8,35,sqrtf(6.0f));
    if(day){ if(snow||snowstorm) glColor3ub(255,255,255); else glColor3ub(101,205,170); }
    else glColor3ub(60,130,100);
    drawCircle(7,39,sqrtf(7.0f));
    glPopMatrix();
}
void drawTree2()
{
    glPushMatrix(); glTranslatef(89,7,0);  drawTree1(); glPopMatrix();
    glPushMatrix(); glTranslatef(89,-15,0); drawTree1(); glPopMatrix();
}

// ── birds ─────────────────────────────────────────────────────────────────────
void drawPolygon(vector<pair<float,float>> coord,float Tx=0,float Ty=0,float s=1,
    unsigned char r=121,unsigned char g=172,unsigned char b=170)
{
    glColor3ub(r,g,b);
    glBegin(GL_POLYGON);
    for(int i=0;i<(int)coord.size();i++) glVertex2f(Tx+s*coord[i].first,Ty+s*coord[i].second);
    glEnd();
}
float birdX=0, birdWingY=-1, birdDirection=1;
float birdBankAngle=0.0f;

void drawBird(float Tx,float Ty,float direction,float wingY,float s=1,
    unsigned char r=121,unsigned char g=172,unsigned char b=170)
{
    glPushMatrix();
    glTranslatef(Tx,Ty,0);
    glRotatef(birdBankAngle*direction, 1,0,0);
    glTranslatef(-Tx,-Ty,0);
    drawPolygon({{0.9f*direction,0.1f},{1.0f*direction,0.05f},{1.0f*direction,0.0f},
        {0.95f*direction,-0.05f},{0.85f*direction,-0.02f},{0.7f*direction,-0.1f},
        {0.3f*direction,-0.2f},{0.1f*direction,-0.2f},{0.0f,-0.3f},
        {0.2f*direction,-0.1f},{0.0f,0.05f},{0.2f*direction,0.0f},
        {0.4f*direction,0.05f},{0.9f*direction,0.1f}},Tx,Ty,s,r,g,b);
    float ew = sinf(birdWingY * 3.14159f * 0.5f);
    drawPolygon({{0.4f*direction,ew*0.05f},{0.45f*direction,ew*0.25f},
        {0.3f*direction,ew*0.8f},{0.4f*direction,ew*0.05f}},Tx,Ty,s,r,g,b);
    glPopMatrix();
}
void drawAllBirds()
{
    drawBird(birdX-5, 58,birdDirection,birdWingY,3,   202,182,49);
    drawBird(birdX-10,54,birdDirection,-birdWingY,2.5,255,134,176);
    drawBird(birdX-15,52,birdDirection,birdWingY,2,   0,0,255);
    drawBird(birdX-20,50,birdDirection,birdWingY,1.5, 0,0,0);
}
void updateBirds(int value)
{
    birdX   += 0.12f * birdDirection;
    birdWingY= fmodf(birdWingY + 0.08f, 2.0f) - 1.0f;
    if(birdX>130) { birdDirection=-1; birdBankAngle=15; }
    if(birdX<-20) { birdDirection= 1; birdBankAngle=15; }
    if(birdBankAngle>0) birdBankAngle-=1.0f;
    glutPostRedisplay(); glutTimerFunc(1000/60, updateBirds, 0);
}

// ── fireflies ─────────────────────────────────────────────────────────────────
const int numFireflies=50;
float fireflyPos[numFireflies][2];
float fireflyPhase[numFireflies];

void initializeFireflies()
{
    srand((unsigned)time(0));
    for(int i=0;i<numFireflies;i++)
    { fireflyPos[i][0]=(float)(rand()%101); fireflyPos[i][1]=(float)(rand()%53+15); fireflyPhase[i]=(float)(rand()%628)/100.0f; }
}
void updateFireflies(int value)
{
    for(int i=0;i<numFireflies;i++)
    {
        fireflyPos[i][0]+=(rand()%5-2)*0.5f;
        fireflyPos[i][1]+=(rand()%5-2)*0.5f;
        if(fireflyPos[i][0]<0)  fireflyPos[i][0]=0;
        if(fireflyPos[i][0]>100)fireflyPos[i][0]=100;
        if(fireflyPos[i][1]<15) fireflyPos[i][1]=15;
        if(fireflyPos[i][1]>67) fireflyPos[i][1]=67;
    }
    glutPostRedisplay(); glutTimerFunc(50, updateFireflies, 0);
}
void drawFireflies()
{
    if(!night||snow||snowstorm) return;
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    for(int i=0;i<numFireflies;i++)
    {
        float x=fireflyPos[i][0], y=fireflyPos[i][1];
        float pulse=0.4f+0.6f*fabsf(sinf(globalTime*1.5f+fireflyPhase[i]));
        glColor4f(1,1,0,pulse);          drawCircle(x,y,0.25f);
        glColor4f(1,1,0.2f,0.25f*pulse); drawCircle(x,y,0.7f);
    }
    glDisable(GL_BLEND);
}

// ── shops ─────────────────────────────────────────────────────────────────────
void drawText(const char* text,float x,float y)
{ glColor3ub(255,255,255); glRasterPos2f(x,y); for(int i=0;text[i];i++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,text[i]); }

void drawShop1()
{
    if(day)glColor3ub(255,255,255);else glColor3ub(100,100,100);
    glBegin(GL_POLYGON);glVertex2f(67.2,10.18);glVertex2f(59.13,10.18);glVertex2f(59.13,19.64);glVertex2f(67.2,19.64);glEnd();
    if(day)glColor3ub(80,154,165);else glColor3ub(40,80,90);
    drawRectangle(67.2,10.18,59.13,10.18,59.13,18.71,67.2,18.71);
    if(day)glColor3ub(255,255,255);else glColor3ub(110,110,110);
    drawRectangle(65.67,12.68,60.59,12.68,60.59,17.63,65.67,17.63);
    if(day)glColor3ub(33,92,104);else glColor3ub(15,50,60);
    drawRectangle(65.29,13.28,61.12,13.28,61.12,17.14,65.29,17.14);
    if(day)glColor3ub(33,92,104);else glColor3ub(15,50,60);
    drawRectangle(59.1,19.64,67.17,19.64,68.64,21.80,60.58,21.80);
    if(day)glColor3ub(24,71,83);else glColor3ub(10,35,45);
    drawTriangle(68.64,21.80,67.21,19.78,70.26,19.78);
    if(day)glColor3ub(30,95,113);else glColor3ub(15,45,55);
    drawRectangle(67.17,19.78,69.79,19.78,69.79,10.25,67.17,10.25);
    if(day)glColor3ub(34,83,93);else glColor3ub(15,40,50);
    drawRectangle(68,14.7,68.9,14.7,68.9,17.78,68,17.78);
}
void drawShop2()
{
    if(day)glColor3ub(229,95,58);else glColor3ub(115,45,30);
    drawRectangle(33.6,23.6,44.91,23.6,44.91,10.12,33.6,10.24);
    if(day)glColor3ub(70,130,180);else glColor3ub(35,65,90);
    drawRectangle(33.6,23.6,44.8,23.6,46.43,27.19,35.57,27.19);
    if(day)glColor3ub(21,63,71);else glColor3ub(10,30,35);
    drawPentagon(44.8,23.6,46.434,27.191,47.67,24.34,47.94,11.22,44.9,10.02);
    if(day)glColor3ub(240,248,255);else glColor3ub(90,100,110);
    drawRectangle(35.22,18.61,40.11,18.6,40.19,12.75,35.3,12.78);
    if(day)glColor3ub(25,25,112);else glColor3ub(12,12,55);
    drawRectangle(35.74,17.46,39.84,17.36,39.91,13.26,35.73,13.29);
    if(day)glColor3ub(28,83,92);else glColor3ub(14,40,45);
    drawRectangle(40.69,18.9,43.13,18.97,43.15,10.15,40.66,10.11);
    float x=0;
    for(int i=0;i<5;i++){
        if(i%2==0){if(day)glColor3ub(255,165,79);else glColor3ub(130,80,40);}
        else{if(day)glColor3ub(139,69,19);else glColor3ub(70,35,15);}
        glBegin(GL_POLYGON);
        glVertex2f(35.23+x,18.97);glVertex2f(36.7+x,18.95);glVertex2f(35.73+x,17.5);
        glVertex2f(35.75+x,16.55);glVertex2f(35.35+x,16.22);glVertex2f(34.98+x,16.18);
        glVertex2f(34.6+x,16.3);glVertex2f(34.37+x,16.65);glVertex2f(34.34+x,17.59);
        glEnd();x+=1.47f;
    }
    drawText("CAFFE",36,20);
}
void drawShop3()
{
    if(day)glColor3ub(96,178,166);else glColor3ub(48,89,83);
    drawRectangle(45.58,10.1,45.58,22.3,56.25,22.11,56.25,10.1);
    if(day)glColor3ub(54,117,123);else glColor3ub(27,58,62);
    drawRectangle(56.25,22.11,56.25,10.1,59,10.1,59,22.11);
    if(day)glColor3ub(38,90,100);else glColor3ub(19,45,50);
    drawRectangle(45.58,22.11,59,22.11,57.72,26.33,47.72,26.33);
    if(day)glColor3ub(25,68,77);else glColor3ub(12,34,38);
    drawTriangle(56.25,22.11,59,22.11,57.72,26.33);
    if(day)glColor3ub(245,245,245);else glColor3ub(100,100,100);
    drawRectangle(46.89,10.1,50.87,10.1,50.87,18.72,46.89,18.72);
    if(day)glColor3ub(70,130,180);else glColor3ub(35,65,90);
    float x=0.52f;
    drawRectangle(46.89+x,10.1+x,50.87-x,10.1+x,50.87-x,18.72-x,46.89+x,18.72-x);
    if(day)glColor3ub(60,130,135);else glColor3ub(30,65,70);
    drawRectangle(46.26,10.57,46.26,10.11,51.51,10.11,51.51,10.57);
    if(day)glColor3ub(245,245,245);else glColor3ub(100,100,100);
    drawRectangle(51.66,13.31,55.04,13.31,55.04,18.75,51.66,18.75);
    if(day)glColor3ub(70,130,180);else glColor3ub(35,65,90);
    drawRectangle(51.66+x,13.31+x,55.04-x,13.31+x,55.04-x,18.75-x,51.66+x,18.75-x);
}
void drawShop4()
{
    if(day)glColor3ub(37,101,113);else glColor3ub(18,50,56);
    drawRectangle(17.05,11.10,17.05,10.16,33.61,10.16,33.7,11.10);
    if(day)glColor3ub(238,123,32);else glColor3ub(120,62,16);
    drawRectangle(17.55,11.10,17.55,23.61,30.84,23.61,30.84,11.10);
    if(day)glColor3ub(205,96,32);else glColor3ub(102,48,16);
    drawRectangle(30.84,23.61,33.61,23.61,33.61,11.10,30.84,11.10);
    if(day)glColor3ub(220,20,60);else glColor3ub(100,10,30);
    drawRectangle(19.6,12.5,19.6,18.41,26.29,18.41,26.29,12.5);
    if(day)glColor3ub(255,99,71);else glColor3ub(130,50,40);
    float y=0.6f;
    drawRectangle(19.6+y,12.5+y,19.6+y,18.41-y,26.29-y,18.41-y,26.29-y,12.5+y);
    if(day)glColor3ub(21,60,69);else glColor3ub(10,25,30);
    drawRectangle(29.2,18.98,26.5,18.98,26.5,11.10,29.2,11.10);
    if(day)glColor3ub(31,90,100);else glColor3ub(15,40,45);
    drawRectangle(17.55,23.61,33.61,23.61,31.85,27.19,19.6,27.19);
    glPushMatrix(); glTranslatef(-16.23f,0,0);
    float x=0;
    for(int i=0;i<6;i++){
        if(i%2==0){if(day)glColor3ub(255,140,0);else glColor3ub(130,70,20);}
        else{if(day)glColor3ub(205,92,92);else glColor3ub(100,40,40);}
        glBegin(GL_POLYGON);
        glVertex2f(35.23+x,18.97);glVertex2f(36.7+x,18.95);glVertex2f(35.73+x,17.5);
        glVertex2f(35.75+x,16.55);glVertex2f(35.35+x,16.22);glVertex2f(34.98+x,16.18);
        glVertex2f(34.6+x,16.3);glVertex2f(34.37+x,16.65);glVertex2f(34.34+x,17.59);
        glEnd();x+=1.47f;
    }
    drawText("BAKERY",37,20);
    glPopMatrix();
}
void drawshop5()
{
    glPushMatrix(); glTranslatef(-41,0,0);
    if(day)glColor3ub(210,105,30);else glColor3ub(105,52,15);
    drawRectangle(45.58,10.1,45.58,22.3,56.25,22.11,56.25,10.1);
    if(day)glColor3ub(160,82,45);else glColor3ub(80,41,22);
    drawRectangle(56.25,22.11,56.25,10.1,59,10.1,59,22.11);
    if(day)glColor3ub(139,69,19);else glColor3ub(70,35,10);
    drawRectangle(45.58,22.11,59,22.11,57.72,26.33,47.72,26.33);
    if(day)glColor3ub(205,92,92);else glColor3ub(100,45,45);
    drawTriangle(56.25,22.11,59,22.11,57.72,26.33);
    if(day)glColor3ub(255,250,205);else glColor3ub(128,128,102);
    drawRectangle(46.89,10.1,50.87,10.1,50.87,18.72,46.89,18.72);
    if(day)glColor3ub(100,149,237);else glColor3ub(50,74,120);
    float x=0.52f;
    drawRectangle(46.89+x,10.1+x,50.87-x,10.1+x,50.87-x,18.72-x,46.89+x,18.72-x);
    if(day)glColor3ub(176,224,230);else glColor3ub(90,110,120);
    drawRectangle(46.26,10.57,46.26,10.11,51.51,10.11,51.51,10.57);
    if(day)glColor3ub(255,250,205);else glColor3ub(128,128,102);
    drawRectangle(51.66,13.31,55.04,13.31,55.04,18.75,51.66,18.75);
    if(day)glColor3ub(100,149,237);else glColor3ub(50,74,120);
    drawRectangle(51.66+x,13.31+x,55.04-x,13.31+x,55.04-x,18.75-x,51.66+x,18.75-x);
    glPopMatrix();
}

// ── buildings ─────────────────────────────────────────────────────────────────
bool light=false;
float windowFlicker[20]={};
void updateWindowFlicker(int value)
{
    for(int i=0;i<20;i++) windowFlicker[i]=(float)(rand()%100)/100.0f;
    glutTimerFunc(800, updateWindowFlicker, 0);
}

void drawBuilding1()
{
    if(day) glColor3ub(210,175,130); else glColor3ub(28,38,52);
    drawRectangle(47.95,25,41.41,25,41.41,45,47.95,45);
    if(day) glColor3ub(185,150,105); else glColor3ub(20,28,40);
    drawRectangle(47.95,25,51,25,51,45,47.95,45);
    if(day) glColor3ub(195,160,115); else glColor3ub(22,32,45);
    drawRectangle(43.38,45,46.78,45,46.78,46.45,43.38,46.45);
    if(day) glColor3ub(170,138,95); else glColor3ub(15,22,32);
    drawRectangle(46.78,45,49.13,45,49.13,46.45,46.78,46.45);
    float colX[4]={42.47f,44.27f,46.07f,49.02f};
    for(int col=0;col<4;col++)
    {
        float wx=colX[col];
        bool flicker= night && !light && windowFlicker[col]>0.9f;
        if(light||flicker)      glColor3ub(255,220,80);
        else if(night)          glColor3ub(17,55,70);
        else                    glColor3ub(95,155,185);
        drawRectangle(wx,42.02,wx+1.01f,42.02,wx+1.01f,43.42,wx,43.42);
        float wy=2.42f;
        for(int i=0;i<6;i++){ drawRectangle(wx,42.02f-wy,wx+1.01f,42.02f-wy,wx+1.01f,43.42f-wy,wx,43.42f-wy); wy+=2.42f; }
    }
}
void drawBuilding2()
{
    if(day) glColor3ub(70,120,175); else glColor3ub(18,28,45);
    drawRectangle(52.51,23,58.23,23,58.23,42.46,52.51,42.46);
    if(day) glColor3ub(50,90,140); else glColor3ub(12,20,33);
    drawRectangle(58.23,23,58.23,42.46,62.71,42.46,62.71,23);
    if(day) glColor3ub(60,105,155); else glColor3ub(20,32,50);
    drawRectangle(53.73,44.62,57.61,44.62,57.61,42.5,53.73,42.5);
    if(day) glColor3ub(40,78,120); else glColor3ub(12,22,35);
    drawRectangle(57.61,44.62,57.61,42.5,60.5,42.5,60.5,44.62);
    float startXArr[3]={53.71f,55.91f,59.61f};
    for(int col=0;col<3;col++)
    {
        float wx=startXArr[col];
        bool flicker= night && !light && windowFlicker[col+4]>0.88f;
        if(light||flicker)      glColor3ub(255,220,80);
        else if(night)          glColor3ub(17,45,65);
        else                    glColor3ub(160,210,240);
        drawRectangle(wx,39.77,wx+1.24f,39.77,wx+1.24f,38.26f,wx,38.26f);
        float wy=2.54f;
        for(int i=0;i<6;i++){ drawRectangle(wx,39.77f-wy,wx+1.24f,39.77f-wy,wx+1.24f,38.26f-wy,wx,38.26f-wy); wy+=2.54f; }
    }
}
void drawbuilding3()
{
    glPushMatrix(); glTranslatef(-14,-2,0);
    if(day) glColor3ub(156,30,30); else glColor3ub(25,45,50);
    drawRectangle(47.95,25,41.41,25,41.41,45,47.95,45);
    if(day) glColor3ub(106,25,25); else glColor3ub(15,35,40);
    drawRectangle(47.95,25,51,25,51,45,47.95,45);
    if(day) glColor3ub(156,30,30); else glColor3ub(25,45,50);
    drawRectangle(43.38,45,46.78,45,46.78,46.45,43.38,46.45);
    if(day) glColor3ub(106,25,25); else glColor3ub(15,35,40);
    drawRectangle(46.78,45,49.13,45,49.13,46.45,46.78,46.45);
    float colX[4]={42.47f,44.27f,46.07f,49.02f};
    for(int col=0;col<4;col++)
    {
        float wx=colX[col];
        bool flicker= night && !light && windowFlicker[col+7]>0.91f;
        if(light||flicker)      glColor3ub(255,220,80);
        else if(night)          glColor3ub(17,55,70);
        else                    glColor3ub(95,130,140);
        drawRectangle(wx,42.02,wx+1.01f,42.02,wx+1.01f,43.42,wx,43.42);
        float wy=2.42f;
        for(int i=0;i<6;i++){ drawRectangle(wx,42.02f-wy,wx+1.01f,42.02f-wy,wx+1.01f,43.42f-wy,wx,43.42f-wy); wy+=2.42f; }
    }
    glPopMatrix();
    glPushMatrix(); glTranslatef(-40,0,0); drawBuilding2(); glPopMatrix();
}

// ── plane ─────────────────────────────────────────────────────────────────────
float planeX=-20;
void plane()
{
    glPushMatrix(); glTranslatef(planeX,0,0);
    if(day)glColor3ub(200,200,200); else glColor3ub(150,150,150);
    glBegin(GL_POLYGON);
    glVertex2f(17.82,56.79);glVertex2f(27.85,58.64);glVertex2f(28.44,58.57);
    glVertex2f(28.93,58.42);glVertex2f(29.41,58.11);glVertex2f(29.96,57.58);
    glVertex2f(30.36,57.19);glVertex2f(30.29,56.59);glVertex2f(29.95,56.33);
    glVertex2f(29.5,56.1);glVertex2f(24.77,55.85);glVertex2f(21.54,55.6);glVertex2f(17.82,56.17);
    glEnd();
    glBegin(GL_POLYGON); glVertex2f(20.57,57);glVertex2f(22.49,57.5);glVertex2f(20.2,59.74);glVertex2f(19.08,59.6); glEnd();
    glBegin(GL_POLYGON); glVertex2f(24.77,55.85);glVertex2f(22,54);glVertex2f(21.21,53.86);glVertex2f(20.48,54.04);glVertex2f(21.54,55.6); glEnd();
    glBegin(GL_POLYGON); glVertex2f(17.82,56.73);glVertex2f(18.7,56.94);glVertex2f(17.8,57.5);glVertex2f(17.32,57.37); glEnd();
    glColor3ub(0,0,0);
    drawCircle(22.52,56.84,0.3f);drawCircle(23.68,56.96,0.3f);drawCircle(24.78,57.08,0.3f);
    drawCircle(25.83,57.175f,0.3f);drawCircle(26.89,57.26,0.3f);
    glBegin(GL_POLYGON); glColor3ub(0,0,0);
    glVertex2f(29.35,58.08);glVertex2f(28.7,58.08);glVertex2f(28.55,57.87);
    glVertex2f(28.57,57.53);glVertex2f(28.72,57.44);glVertex2f(30.08,57.53);
    glEnd();
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1,1,1,0.15f);
    glBegin(GL_TRIANGLES); glVertex2f(17.5,57.2); glVertex2f(5,60); glVertex2f(5,54); glEnd();
    glDisable(GL_BLEND);
    glPopMatrix();
}
void updateplane(int value)
{
    planeX+=1; if(planeX>100)planeX=-30;
    glutPostRedisplay(); glutTimerFunc(100, updateplane, 0);
}

// ── car ───────────────────────────────────────────────────────────────────────
void drawRotatedEllipse(float cx,float cy,float rx,float ry,float angleDeg,int seg=100)
{
    float ar=angleDeg*M_PI/180.0f;
    glBegin(GL_POLYGON);
    for(int i=0;i<seg;i++)
    {
        float t=2.0f*M_PI*i/seg, x=rx*cosf(t), y=ry*sinf(t);
        glVertex2f(cx+x*cosf(ar)-y*sinf(ar), cy+x*sinf(ar)+y*cosf(ar));
    }
    glEnd();
}
float carX=-40;
float wheelSpin=0;
bool Red=false, Yellow=false, Green=true;
void drawCar()
{
    glPushMatrix(); glTranslatef(carX,0,0);
    if(day)glColor3ub(246,130,29); else glColor3ub(130,80,40);
    glBegin(GL_POLYGON);
    glVertex2f(24,6);glVertex2f(22.84,6.15);glVertex2f(22.32,6.74);glVertex2f(22.456,7.466);
    glVertex2f(22.82,7.759);glVertex2f(22.86,8.99);glVertex2f(23.29,9.53);glVertex2f(23.86,10.01);
    glVertex2f(26.84,12.62);glVertex2f(27.64,13.03);glVertex2f(34.98,13.14);glVertex2f(36.09,12.75);
    glVertex2f(38.74,10.31);glVertex2f(41.09,9.91);glVertex2f(42.09,9.57);glVertex2f(42.52,9.31);
    glVertex2f(42.95,8.64);glVertex2f(43.44,8.18);glVertex2f(43.72,7.60);glVertex2f(43.76,6.99);
    glVertex2f(43.63,6.44);glVertex2f(43.19,6.05);
    glEnd();
    glColor3ub(0,0,0);
    glBegin(GL_POLYGON); glVertex2f(25.92,10.19);glVertex2f(26,11);glVertex2f(26.44,11.58);glVertex2f(27.07,12.06);glVertex2f(27.06,12.29);glVertex2f(28.12,12.48);glVertex2f(29.66,12.59);glVertex2f(29.66,10.19); glEnd();
    glBegin(GL_POLYGON); glVertex2f(30.34,10.22);glVertex2f(30.31,12.50);glVertex2f(32.1,12.52);glVertex2f(32.63,12.39);glVertex2f(33.00,12.16);glVertex2f(33.41,11.87);glVertex2f(33.71,11.60);glVertex2f(34.05,11.19);glVertex2f(34.41,10.85);glVertex2f(34.81,10.21); glEnd();
    glBegin(GL_POLYGON); glVertex2f(36.15,10.33);glVertex2f(38.80,10.27);glVertex2f(36.83,12.19);glVertex2f(36.34,12.50);glVertex2f(34.17,12.68);glVertex2f(34.16,12.22); glEnd();
    float wx[2]={26.5f,36.77f};
    for(int i=0;i<2;i++)
    {
        glColor3ub(0,0,0); drawCircle(wx[i],6.25f,1.9f);
        glColor3ub(80,80,80); drawCircle(wx[i],6.25f,1.1f);
        glColor3ub(140,140,140); glLineWidth(1.5f);
        glPushMatrix(); glTranslatef(wx[i],6.25f,0); glRotatef(wheelSpin,0,0,1);
        glBegin(GL_LINES); glVertex2f(-1.0f,0); glVertex2f(1.0f,0); glVertex2f(0,-1.0f); glVertex2f(0,1.0f); glEnd();
        glPopMatrix(); glLineWidth(1.0f);
    }
    drawRectangle(41.44,7.82,41.40,8.47,42.81,8.45,42.83,7.92);
    drawRectangle(40.64,6.49,40.62,7.00,43.23,7.07,43.25,6.56);
    glColor3f(1,1,0.8f); drawRotatedEllipse(39.65f,8.3f,1,0.7f,-35);
    if(!day)
    {
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1,1,0.8f,0.3f);
        glBegin(GL_TRIANGLES); glVertex2f(39.88,8.7); glVertex2f(59,12); glVertex2f(59,4); glEnd();
        glDisable(GL_BLEND);
    }
    glColor3f(0,0,0);
    drawRectangle(31.34,9.27,31.39,9.51,30.5,9.5,30.48,9.27);
    drawRectangle(27.20,9.27,27.10,9.62,26.40,9.60,26.38,9.27);
    glColor3f(1,0,0);
    drawRectangle(22.84,7.66,23.36,8.04,23.38,8.83,22.86,8.83);
    glPopMatrix();
}

void drawTrafficLight()
{
    glColor3ub(0,0,0);
    glBegin(GL_POLYGON); glVertex2f(73,9);glVertex2f(74,9);glVertex2f(74,15);glVertex2f(73,15); glEnd();
    glBegin(GL_POLYGON); glVertex2f(74.6,15);glVertex2f(72.4,15);glVertex2f(72.4,20);glVertex2f(74.6,20); glEnd();
    if(Red)   glColor3ub(255,0,0);   else glColor3ub(100,0,0);   drawCircle(73.5,16.3,0.5f);
    if(Yellow)glColor3ub(255,255,0); else glColor3ub(100,100,0); drawCircle(73.5,17.6,0.5f);
    if(Green) glColor3ub(0,255,0);   else glColor3ub(0,100,0);   drawCircle(73.5,18.9,0.5f);
}
void drawBench()
{
    glPushMatrix(); glTranslatef(0,4,0);
    glColor3f(0.4f,0.2f,0.1f);
    drawQuad(79,9.5,89,9.5,89,10.5,79,10.5); drawQuad(79,11,89,11,89,12,79,12);
    glColor3f(0.3f,0.2f,0.1f);
    drawQuad(79,8,79.5,8,79.5,11,79,11); drawQuad(88.5,8,89,8,89,11,88.5,11);
    glColor3f(0.5f,0.3f,0.1f);
    drawQuad(78.5,7.5,89.5,7.5,89.5,8.5,78.5,8.5);
    glColor3f(0.3f,0.2f,0.1f);
    drawQuad(79.5,7.5,80,7.5,80,6,79.5,6); drawQuad(88,7.5,88.5,7.5,88.5,6,88,6);
    glPopMatrix();
}

bool fast=false;
void car_update(int value)
{
    if(Green){ if(fast)carX+=2; else carX+=1; wheelSpin+=fast?20:10; }
    else if(Yellow){ carX+=0.5f; wheelSpin+=5; }
    if(carX>100)carX=-40;
    if(wheelSpin>360)wheelSpin-=360;
    glutPostRedisplay(); glutTimerFunc(50, car_update, 0);
}

// ── tree1 / tree2 ─────────────────────────────────────────────────────────────
void tree1()
{
    glBegin(GL_POLYGON);
    if(day)glColor3ub(101,67,33); else glColor3ub(60,40,20);
    glVertex2f(68.9,26.6);glVertex2f(70.4,26.6);glVertex2f(70.4,24.5);glVertex2f(68.9,24.5);
    glEnd();
    glPushMatrix(); glTranslatef(69.65,26.6,0);
    float sway = snowstorm ? 6.0f*sinf(treeAngle*1.2f) : (snow ? 2.5f*sinf(treeAngle*0.6f) : 0.5f*sinf(treeAngle*0.3f));
    glRotatef(sway,0,0,1);
    glTranslatef(-69.65,-26.6,0);
    if(day)glColor3ub(34,139,34); else glColor3ub(20,80,50);
    glBegin(GL_POLYGON); glVertex2f(65.35,26.6);glVertex2f(73.91,26.6);glVertex2f(72.17,30);glVertex2f(67.19,30); glEnd();
    if(day)glColor3ub(46,138,87); else glColor3ub(20,60,40);
    glBegin(GL_POLYGON); glVertex2f(66,30);glVertex2f(73.39,30);glVertex2f(71.58,33.4);glVertex2f(67.84,33.4); glEnd();
    if(day)glColor3ub(60,179,113); else glColor3ub(35,90,60);
    glBegin(GL_POLYGON); glVertex2f(66.79,33.4);glVertex2f(72.61,33.4);glVertex2f(70.83,36.89);glVertex2f(68.45,36.89); glEnd();
    if(day){if(snow||snowstorm)glColor3ub(240,248,255); else glColor3ub(101,205,170);}
    else glColor3ub(60,130,100);
    glBegin(GL_POLYGON); glVertex2f(67.63,36.89);glVertex2f(71.8,36.89);glVertex2f(69.7,41.2); glEnd();
    glPopMatrix();
}
void treeUpdate(int value)
{
    treeAngle+=0.1f; if(treeAngle>2*3.14159f)treeAngle-=2*3.14159f;
    glutPostRedisplay(); glutTimerFunc(30, treeUpdate, 0);
}
void tree2()
{
    glPushMatrix(); glScalef(0.75,0.75,0); glTranslatef(32,12,0); tree1(); glPopMatrix();
}

// ── global time update ────────────────────────────────────────────────────────
void updateGlobalTime(int value)
{
    globalTime += 0.016f;
    updateAmbientSound();
    glutPostRedisplay(); glutTimerFunc(16, updateGlobalTime, 0);
}

// ── display ───────────────────────────────────────────────────────────────────
void display()
{
    glClearColor(1,1,1,1);
    glClear(GL_COLOR_BUFFER_BIT);
    sky();
    backside();
    drawHill2(); drawHill(); drawHll3();

    glBegin(GL_POLYGON);
    glColor3ub(30,144,255);
    glVertex2f(10,31);glVertex2f(65,31);glVertex2f(65,25);glVertex2f(10,25);
    glEnd();

    int r,g,b;
    if(day||afternoon) r=97,  g=170, b=50;
    else               r=38,  g=88,  b=34;
    r=int(r+(255-r)*snowIntensity);
    g=int(g+(255-g)*snowIntensity);
    b=int(b+(255-b)*snowIntensity);
    if(afternoonBlend > 0.0f)
    {
        r=int(r + afternoonBlend * (210 - r) * 0.25f);
        g=int(g + afternoonBlend * (175 - g) * 0.15f);
        b=int(b - afternoonBlend * b * 0.20f);
        if(b<0)b=0;
    }
    glColor3ub(r,g,b);
    drawGound();
    if(r>220) drawSnowman(5);

    Road();
    drawAllBirds();
    drawTree1();

    int baseR=30,baseG=120,baseB=210;
    int iceR=160,iceG=210,iceB=230;
    int r1=baseR+int((iceR-baseR)*snowIntensity);
    int g1=baseG+int((iceG-baseG)*snowIntensity);
    int b1=baseB+int((iceB-baseB)*snowIntensity);
    drawLake(r1,g1,b1);

    drawFireflies();
    if(snow||snowstorm) drawSnow();
    drawWindmill();
    drawLightningFlash();
    drawBoat();
    drawBuilding1(); drawBuilding2(); drawbuilding3();
    drawshop5(); drawShop1(); drawShop2(); drawShop3(); drawShop4();
    plane();
    tree1(); tree2(); drawTree2();
    drawTrafficLight(); drawBench();
    drawCar();
    drawAfternoonOverlay();
    glFlush();
}

// ── input ─────────────────────────────────────────────────────────────────────
void handleKeypress(unsigned char key,int x,int y)
{
    if(key=='s'||key=='S')
    {
        snow=!snow; snowstorm=false;
        snowSoundPlaying=false; snowstormSoundPlaying=false;
        if(snow){initSnowflakes(); glutTimerFunc(0,updateSnow,0);}
        else { snowflakes.clear(); stopAllSounds(); snowSoundPlaying=false; }
        glutPostRedisplay();
    }
    else if(key=='w'||key=='W')
    {
        snowstorm=!snowstorm; snow=false;
        snowstormSoundPlaying=false; snowSoundPlaying=false;
        if(snowstorm){initSnowflakes(); glutTimerFunc(0,updateSnow,0);}
        else { snowflakes.clear(); stopAllSounds(); snowstormSoundPlaying=false; }
        glutPostRedisplay();
    }
    else if(key=='e'||key=='E'){snow=false;snowstorm=false;snowflakes.clear();stopAllSounds();snowSoundPlaying=false;snowstormSoundPlaying=false;glutPostRedisplay();}
    else if(key=='n'||key=='N'){if(!night){night=true;day=false;afternoon=false;goingToNight=true;transitioning=true;glutPostRedisplay();}}
    else if(key=='d'||key=='D'){if(!day){day=true;night=false;afternoon=false;goingToNight=false;transitioning=true;nightAmbientPlaying=false;stopAllSounds();glutPostRedisplay();}}
    else if(key=='a'||key=='A')
    {
        afternoon=!afternoon;
        if(afternoon){ day=true; night=false; goingToNight=false; transitioning=false; }
        glutPostRedisplay();
    }
    else if(key=='l'||key=='L'){waterLabel2=false;waterLabel1=true;lakeX=1;glutPostRedisplay();}
    else if(key=='q'||key=='Q'){waterLabel1=false;waterLabel2=true;lakeY=1.4f;glutPostRedisplay();}
    else if(key=='r'||key=='R'){Red=true;Yellow=false;Green=false;glutPostRedisplay();}
    else if(key=='y'||key=='Y'){Red=false;Yellow=true;Green=false;glutPostRedisplay();}
    else if(key=='g'||key=='G'){Red=false;Yellow=false;Green=true;glutPostRedisplay();}
}
void mouse(int button,int state,int x,int y)
{
    if(state==GLUT_DOWN)
    {
        if(button==GLUT_LEFT_BUTTON)        light=!light;
        else if(button==GLUT_RIGHT_BUTTON)  fast=true;
        else if(button==GLUT_MIDDLE_BUTTON) fast=false;
        glutPostRedisplay();
    }
}

void Update_Timer_Function()
{
    glutTimerFunc(100,     updateClouds,         0);
    glutTimerFunc(100,     updateSnowIntensity,  0);
    glutTimerFunc(150,     updateLightningFlash, 0);
    glutTimerFunc(100,     updateSunMoon,        0);
    glutTimerFunc(40,      updateLake,           0);
    glutTimerFunc(1000/60, updateBirds,          0);
    glutTimerFunc(50,      updateFireflies,      0);
    glutTimerFunc(16,      windmillUpdate,       0);
    glutTimerFunc(100,     updateBoat,           0);
    glutTimerFunc(100,     updateplane,          0);
    glutTimerFunc(50,      car_update,           0);
    glutTimerFunc(30,      treeUpdate,           0);
    glutTimerFunc(16,      updateGlobalTime,     0);
    glutTimerFunc(800,     updateWindowFlicker,  0);
    glutTimerFunc(40,      updateAfternoon,      0);
}

int main(int argc,char** argv)
{
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_SINGLE|GLUT_RGB);
    glutInitWindowSize(1000,700);
    glutInitWindowPosition(100,100);
    glutCreateWindow("OpenGL Scene");
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0,100.0,0.0,67.0);
    srand((unsigned)time(0));
    initializeFireflies();
    glutDisplayFunc(display);
    glutKeyboardFunc(handleKeypress);
    glutMouseFunc(mouse);
    Update_Timer_Function();
    glutMainLoop();
    return 0;
}

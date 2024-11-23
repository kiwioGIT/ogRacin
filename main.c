#include <SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 360

#define new_max(x,y) (((x) >= (y)) ? (x) : (y))
#define new_min(x,y) (((x) <= (y)) ? (x) : (y))

#define FRAME_LENGHT_FLOOR 0.017
#define LOG_ENABLED 1
#define LOG_FRAME_LENGHT 0
#define LOG_SPEED 0
#define LOG_TIMING 60

const float screenScale = 1;
float lapTime = 9999999999;
int notInStart = 1;


struct Float3{
    float x;
    float y;
    float z;
};

struct ORC_PlayerState{
    struct Float3 pos;
    struct Float3 velo;
    float rot;
    float sinRot;
    float cosRot;
};

struct ORC_Input{
    int xA;
    int yA;
    int zA;
    int rot;
    float sinRot;
    float cosRot;
	bool quit;
};

struct ORC_Settings{
    float scaleX;
    float scaleY;
    float rotSpeed;
    float moveSpeed;
    float playerDistance;
    float forwardPlayerDrag;
    float lateralPlayerDrag;
    float playerAcceleration;
    float speedCap;
};

struct ORC_CamState{
    struct Float3 pos;
    float rot;
    float sinRot;
    float cosRot;
};

bool init(SDL_Window ** gWindow, SDL_Surface ** gScreenSurface);


void setPixel(SDL_Surface  *surface, int x, int y, Uint32 pixel);

void applyPhysics(struct ORC_PlayerState * playerState, struct ORC_Settings * gameSettings, struct ORC_Input * input, SDL_Surface * gameMaps[4]);

Uint32 getPixel(SDL_Surface *surface, int x, int y);

void getInput(struct ORC_Input * inputOut,SDL_Event * event);

void updateCam(struct ORC_CamState * camState, struct ORC_PlayerState * playerState, struct ORC_Settings * gameSettings);

float dot(float x1, float y1, float x2, float y2){
    return x1*x2 + y1*y2;
}

float lenght(float x, float y){
    return sqrt(x*x + y*y);
}

float lenghtSquared(float x, float y){
    return (x*x + y*y);
}

float lerp(float v0, float v1, float t) {
    return (1 - t) * v0 + t * v1;
}

void startLoop(SDL_Window * window, SDL_Surface * screenSurface, SDL_Surface * gameMaps[4], struct ORC_Settings * gameSettings, struct ORC_CamState * camState, struct ORC_PlayerState * playerState, SDL_Renderer * renderer)
{

	SDL_Event e;
	struct ORC_Input input = {0, 0, 0, false};

    SDL_PixelFormat * pFormat = (*screenSurface).format;
    if (pFormat == NULL){
        printf( "SDL Error: %s\n", SDL_GetError() );
    }

    Uint64 NOW = SDL_GetPerformanceCounter();
    Uint64 LAST = 0;

    double deltaTime = 0;
    Uint16 frame = 0;

    float pImgScale = SCREEN_HEIGHT * 4 / 720;

    SDL_Surface * kartSurf = SDL_LoadBMP("kart.bmp");

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_BITSPERPIXEL(8), SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

    SDL_Texture * kartTex = SDL_CreateTextureFromSurface(renderer, kartSurf);

    if( texture == NULL ){printf( "SDL Error: %s\n", SDL_GetError() );}

    SDL_Rect dst = {SCREEN_WIDTH * screenScale / 2 - pImgScale * kartSurf->w / 2,3 * SCREEN_HEIGHT * screenScale / 4 - pImgScale * kartSurf->h / 2,pImgScale * kartSurf->w  * screenScale,pImgScale * kartSurf->h  * screenScale};

while (!input.quit){

    frame++;

    LAST = NOW;
    NOW = SDL_GetPerformanceCounter();
    deltaTime = (double)((NOW - LAST) / (double)SDL_GetPerformanceFrequency() );
    usleep(new_max(0, 1000000 * (FRAME_LENGHT_FLOOR - deltaTime)));
    NOW = SDL_GetPerformanceCounter();

    SDL_LockTextureToSurface(texture, NULL, &screenSurface);
    getInput(&input, &e);
    applyPhysics(playerState, gameSettings, &input, gameMaps);
    updateCam(camState, playerState, gameSettings);


    //Uint8 rComp;
    //SDL_GetRGB(keyPixel, rComp, NULL, NULL, NULL);
    //printf("%i\n", keyPixel);


	for (int y = 0; y < SCREEN_HEIGHT / 2; y++){
	    for (int x = 0; x < SCREEN_WIDTH; x++){
			if (y != 0){
			    float mappedX = camState->pos.z * ((x - SCREEN_WIDTH / 2) * gameSettings->scaleX) / y;
			    float mappedY = camState->pos.z * gameSettings->scaleY / y;

                int rotatedX = camState->cosRot * mappedX - camState->sinRot * mappedY  + camState->pos.x;
                int rotatedY = camState->sinRot * mappedX + camState->cosRot * mappedY  + camState->pos.y;

                if (rotatedX > gameMaps[0]->w || rotatedX <= 0 || rotatedY > gameMaps[0]->h || rotatedY < 0){
                    setPixel(screenSurface, x, y + SCREEN_HEIGHT / 2, SDL_MapRGB(screenSurface->format, 0, 40, 0));
                }
                else{
			        Uint32 pixel = getPixel(gameMaps[0], rotatedX, rotatedY);
			        setPixel(screenSurface, x, y + SCREEN_HEIGHT / 2, pixel);
                }
            }
		}
	}


    SDL_UnlockTexture(texture);
    SDL_RenderClear(renderer);


    SDL_RenderCopy(renderer, texture, NULL , NULL);
    SDL_RenderCopy(renderer, kartTex, NULL, &dst);
    SDL_RenderPresent(renderer);

#if LOG_ENABLED == 1
    if (frame%LOG_TIMING == 0){
        //printf("%f", camState->rot);
#if LOG_FRAME_LENGHT == 1
        printf("Calculation length: %f ms, %f %% Time ulitlisation\n", deltaTime * 1000, 100 * (deltaTime) / frameTimeCap);
#endif
#if LOG_SPEED == 1
        printf("Speed: %f\n", lenght(playerState->velo.x,playerState->velo.y));
#endif
    }
#endif

}
}


int main( int argc, char * args[] )
{
	SDL_Window * gWindow = NULL;

	SDL_Surface * gScreenSurface = NULL;

    gScreenSurface = SDL_CreateRGBSurface(0,SCREEN_WIDTH, SCREEN_HEIGHT, 8, 0,0,0,0);
    if (gScreenSurface == NULL){
        printf( "SDL Error: %s\n", SDL_GetError() );
    }

	bool launchSuccess = init(&gWindow, &gScreenSurface);


    SDL_Renderer *renderer = SDL_CreateRenderer( gWindow, -1, SDL_RENDERER_ACCELERATED);

    if( renderer == NULL )
    {
        printf( "Renderer got himself absolutely demolished! SDL Error: %s\n", SDL_GetError() );
    }

    SDL_SetRenderDrawColor( renderer, 0xFF, 0xFF, 0xFF, 0xFF );

	SDL_Surface * loadedMaps[4];

	loadedMaps[0] = SDL_LoadBMP("marioKartMap.bmp");

    loadedMaps[1] = SDL_LoadBMP("marioKartMapKey.bmp");

	struct ORC_Settings settings = {1 ,400, -0.018, 0,720 * 25 / SCREEN_HEIGHT, 0.997, 0.983, 0.03, 4.0}; // cam scale x, cam scale y, rot speed, move speed, player distance, forward player drag ,lateral player drag, player accel, top speed
	struct ORC_CamState camState = {0,0,14}; // x - horizontal, y - horizontal, fake "z" vertical , rottation

    struct Float3 pPos = {150 , 150 ,14};
    struct Float3 pVelo = {0.0 , 0.0 ,0.0};
    struct ORC_PlayerState pState = {pPos, pVelo};
    pState.velo = pVelo;

	startLoop(gWindow, gScreenSurface,loadedMaps, &settings, &camState, &pState, renderer);

	SDL_DestroyWindow( gWindow );

	SDL_Quit();

	return 0;
}


bool init(SDL_Window ** gWindow, SDL_Surface ** gScreenSurface)
{
    bool success = true;
    printf("----------------------\n");
    if( SDL_Init( SDL_INIT_VIDEO ) < 0 ){
        printf( "SDL INIT FAILED : %s\n", SDL_GetError() );
        success = false;
    }
    else{
        *gWindow = SDL_CreateWindow( "OGRACER", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH * screenScale, SCREEN_HEIGHT * screenScale, SDL_WINDOW_SHOWN );
        printf("SDL INIT OK\n");
		if( *gWindow == NULL ){
            printf( "SDL WINDOW IS COOKED: %s\n", SDL_GetError() );
            success = false;
        }
    }

#if LOG_ENABLED == 1
    printf("GAME LOG ENABLED\n");
    printf("SPEED LOG       : %i\n", LOG_SPEED);
    printf("FRAME TIMES LOG : %i\n", LOG_FRAME_LENGHT);
    printf("----------------------\n");

#endif
    return success;
}

void setPixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
  Uint32 * const target_pixel = (Uint32 *) ((Uint8 *) surface->pixels
                                             + y * surface->pitch
                                             + x * surface->format->BytesPerPixel);
  *target_pixel = pixel;
}

Uint32 getPixel(SDL_Surface *surface, int x, int y)
{
  Uint32 * const target_pixel = (Uint32 *) ((Uint8 *) surface->pixels
                                             + y * surface->pitch
                                             + x * surface->format->BytesPerPixel);
  return *target_pixel;
}

void applyPhysics(struct ORC_PlayerState * playerState, struct ORC_Settings * gameSettings, struct ORC_Input * input, SDL_Surface * gameMaps[4]){

    // PRE COMPUTING
    float sn = sin(playerState->rot);
    float cs = cos(playerState->rot);

    float fSpeed = dot(playerState->velo.x, playerState->velo.y, sn, -cs);
    float lSpeed = dot(playerState->velo.x, playerState->velo.y, cs, sn);

    float speed = lenght(playerState->velo.x, playerState->velo.y);

    playerState->sinRot = sn;
    playerState->cosRot = cs;

    // DRAG

    float nFwD = 1.0 - gameSettings->forwardPlayerDrag;
    float nLtD = 1.0 - gameSettings->lateralPlayerDrag;

    if (input->yA > 0){
        nLtD /= 2;
        nFwD /= 2;
    }
    Uint32 keyPixel = getPixel(gameMaps[1], playerState->pos.x, playerState->pos.y);
    if (keyPixel == -65536){
        nFwD *= 13;
        nLtD *= 13;
    }
    if (keyPixel == -16711936){
        if (notInStart > 2){
            printf("Lap time was: %f\n", notInStart * FRAME_LENGHT_FLOOR);
        }
        notInStart = 0;
    }
    else{
        notInStart += 1;
    }

    nFwD = 1.0 - nFwD;
    nLtD = 1.0 - nLtD;

    fSpeed *= nFwD;
    lSpeed *= nLtD;

    // reasemble
    playerState->velo.x = sn * fSpeed + cs * lSpeed;
    playerState->velo.y = -cs * fSpeed + sn * lSpeed;

    // ACCELERATION
    float nPlayerAcceleration = new_min(gameSettings->playerAcceleration, gameSettings->playerAcceleration / fabs(fSpeed));

    float moveX = input->xA * nPlayerAcceleration;
    float moveY = input->yA * nPlayerAcceleration;

    playerState->velo.x += cs * moveX - sn * moveY;
    playerState->velo.y += sn * moveX + cs * moveY;

    // MOVEMENT

    Uint32 keyPixelAhead = getPixel(gameMaps[1], playerState->pos.x + playerState->velo.x, playerState->pos.y + playerState->velo.y);

    if (keyPixelAhead != -1){
        playerState->pos.x += playerState->velo.x;
        playerState->pos.y += playerState->velo.y;
        //playerState->pos.z += input->zA * gameSettings->moveSpeed; 
    }
    else{
        playerState->velo.x = 0;
        playerState->velo.x = 0;
    }


    // ROTATION
    float nRotSpeed = new_min(gameSettings->rotSpeed,-gameSettings->rotSpeed / speed);
    playerState->rot += input->rot * nRotSpeed;

    // SPEED CAPPING
    float nSpeedCap = gameSettings->speedCap;

    if (lenghtSquared(playerState->velo.x,playerState->velo.y) > nSpeedCap * nSpeedCap){
        float l = lenght(playerState->velo.x,playerState->velo.y);
        playerState->velo.x = nSpeedCap * playerState->velo.x / l;
        playerState->velo.y = nSpeedCap * playerState->velo.y / l;
    }
}

void updateCam(struct ORC_CamState * camState, struct ORC_PlayerState * playerState, struct ORC_Settings * gameSettings){
    camState->pos = playerState->pos;

    float l = lenght(playerState->velo.x , playerState->velo.y);

    //float difX = playerState->velo.x / l;
    //float difY = playerState->velo.y / l;

    //printf("%f\n", lenght(difX, difY));
    camState->pos.x += /*-difX * gameSettings->playerDistance;//*/playerState->sinRot * gameSettings->playerDistance;
    camState->pos.y -= /*-difY * gameSettings->playerDistance ;//*/playerState->cosRot * gameSettings->playerDistance;
    //camState->rot = asin(-difX / difY);
    //camState->rot = 0;

    camState->sinRot = playerState->sinRot;
    camState->cosRot = playerState->cosRot;
}

void getInput(struct ORC_Input * inputOut,SDL_Event * event){
	Uint8* keystate = SDL_GetKeyboardState(NULL);
    (inputOut)->xA = 0;
    (inputOut)->zA = 0;
    (inputOut)->yA = 0;
    (inputOut)->rot = 0;
    (inputOut)->quit = false;

	if(keystate[SDL_SCANCODE_A] == 1){
        //(inputOut)->xA -= 1;
    }
    if(keystate[SDL_SCANCODE_D]){
        //(inputOut)->xA += 1;
    }
    if(keystate[SDL_SCANCODE_UP]){
        (inputOut)->yA += 1;
    }
    if(keystate[SDL_SCANCODE_DOWN]){
        (inputOut)->yA -= 1;
    }

    if(keystate[SDL_SCANCODE_LEFT]){
        (inputOut)->rot -= 1;
    }
    if(keystate[SDL_SCANCODE_RIGHT]){
        (inputOut)->rot += 1;
    }
    if(keystate[SDL_SCANCODE_SPACE]){
        //(inputOut)->zA += 1;
    }
    if(keystate[SDL_SCANCODE_LSHIFT]){
        //(inputOut)->zA -= 1;
    }

    while(SDL_PollEvent(event)){
        switch ((*event).type){
            case SDL_QUIT:
                inputOut->quit = true;
            case SDL_KEYDOWN:
                if((*event).key.keysym.sym == SDLK_ESCAPE)
                    inputOut->quit = true;
            break;
        }
    }
}

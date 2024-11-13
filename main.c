#include <SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>

#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 720
#define PIXEL_SCALE = 4

#define new_max(x,y) (((x) >= (y)) ? (x) : (y))
#define new_min(x,y) (((x) <= (y)) ? (x) : (y))

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
};

bool init(SDL_Window ** gWindow, SDL_Surface ** gScreenSurface);

void setPixel(SDL_Surface  *surface, int x, int y, Uint32 pixel);

void applyPhysics(struct ORC_PlayerState * playerState, struct ORC_Settings * gameSettings, struct ORC_Input * input);

Uint32 getPixel(SDL_Surface *surface, int x, int y);

void getInput(struct ORC_Input * inputOut,SDL_Event * event);

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

    float pImgScale = 4;

    SDL_Surface * kartSurf = SDL_LoadBMP("kart.bmp"); 

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_BITSPERPIXEL(8), SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

    SDL_Texture * kartTex = SDL_CreateTextureFromSurface(renderer, kartSurf);

    if( texture == NULL ){printf( "SDL Error: %s\n", SDL_GetError() );}

    SDL_Rect dst = {SCREEN_WIDTH / 2 - pImgScale * kartSurf->w / 2,3 * SCREEN_HEIGHT / 4 - pImgScale * kartSurf->h / 2,pImgScale * kartSurf->w,pImgScale * kartSurf->h};

while (!input.quit){
    
    frame++;

    LAST = NOW;
    NOW = SDL_GetPerformanceCounter();
    deltaTime = (double)((NOW - LAST) / (double)SDL_GetPerformanceFrequency() );
    double frameTimeCap = 0.008;
    usleep(new_max(0, 1000000 * (frameTimeCap - deltaTime)));
    NOW = SDL_GetPerformanceCounter();
    
    SDL_LockTextureToSurface(texture, NULL, &screenSurface);
    getInput(&input, &e);
    applyPhysics(playerState, gameSettings, &input);
    updateCam(camState, playerState, gameSettings);


    if (frame%120 == 0){
        printf("%f\n", lenght(playerState->velo.x, playerState->velo.y));
        //printf("%f\n", camState->pos.z);
    }

    
	for (int y = 0; y < SCREEN_HEIGHT / 2; y++){
	    for (int x = 0; x < SCREEN_WIDTH; x++){
			if (y != 0){
			    float mappedX = camState->pos.z * ((x - SCREEN_WIDTH / 2) * gameSettings->scaleX) / y;
			    float mappedY = camState->pos.z * gameSettings->scaleY / y;

                int rotatedX = playerState->cosRot * mappedX - playerState->sinRot * mappedY  + camState->pos.x;
                int rotatedY = playerState->sinRot * mappedX + playerState->cosRot * mappedY  + camState->pos.y;

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
    SDL_RenderClear( renderer );

    
    SDL_RenderCopy(renderer, texture, NULL , NULL);
    SDL_RenderCopy(renderer, kartTex, NULL, &dst);
    SDL_RenderPresent( renderer );
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

	struct ORC_Settings settings = {1 ,400, -0.01, 0, 25, 0.999, 0.988, 0.007, 2.0}; // cam scale x, cam scale y, rot speed, move speed, player distance, forward player drag ,lateral player drag, player accel, top speed
	struct ORC_CamState camState = {0,0,14}; // x - horizontal, y - horizontal, fake "z" vertical , rottation

    struct Float3 pPos = {150 , 0 ,14};
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

    if( SDL_Init( SDL_INIT_VIDEO ) < 0 ){
        printf( "SDL INIT FAILED : %s\n", SDL_GetError() );
        success = false;
    }
    else{
        *gWindow = SDL_CreateWindow( "OGRACER", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
        printf("SDL INIT OK\n");
		if( *gWindow == NULL ){
            printf( "SDL WINDOW IS COOKED: %s\n", SDL_GetError() );
            success = false;
        }
    }

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

void applyPhysics(struct ORC_PlayerState * playerState, struct ORC_Settings * gameSettings, struct ORC_Input * input){

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
        nLtD /= 2.5;
        nFwD /= 2;
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
    playerState->pos.x += playerState->velo.x;
    playerState->pos.y += playerState->velo.y;
    //playerState->pos.z += input->zA * gameSettings->moveSpeed; 

    // ROTATION
    float nRotSpeed = new_min(gameSettings->rotSpeed, gameSettings->rotSpeed / speed);
    playerState->rot += input->rot * gameSettings->rotSpeed;

    
    


    //float nPlayerDrag = gameSettings->playerDrag;

    //if (input->yA == 1){
    //    nPlayerDrag = (nPlayerDrag + 1) / 2;
    //}

    //playerState->velo.x *= nPlayerDrag;
    //playerState->velo.y *= nPlayerDrag;
    

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
    camState->pos.x += playerState->sinRot * gameSettings->playerDistance;
    camState->pos.y -= playerState->cosRot * gameSettings->playerDistance;
    camState->rot = playerState->rot;
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


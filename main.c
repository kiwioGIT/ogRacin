#include <SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

bool init(SDL_Window ** gWindow, SDL_Surface ** gScreenSurface);

void setPixel(SDL_Surface  *surface, int x, int y, Uint32 pixel);

Uint32 getPixel(SDL_Surface *surface, int x, int y);

int ypos = 0;

struct ORInput{
    int xA;
    int yA;
    int zA;

    int rot;
    float sinRot;
    float cosRot;

	bool quit;
};

void applyInput(struct ORInput * in, float (*camState)[4], float (*gameSettings)[8]);

void getInput(struct ORInput * inputOut,SDL_Event * event);

void startLoop(SDL_Window * window, SDL_Surface * screenSurface, SDL_Surface * gameMaps[4], float (*gameSettings)[8], float (*camState)[4])
{
	SDL_Event e;
	struct ORInput input = {0, 0, 0, false};
    SDL_PixelFormat * pFormat = (*screenSurface).format;
	while (!input.quit){
    
	getInput(&input, &e);
    applyInput(&input, camState, gameSettings);

	for (int y = 0; y < SCREEN_HEIGHT / 2; y++){
		for (int x = 0; x < SCREEN_WIDTH; x++){
			if (y == 0){continue;}

			float mappedX = (*camState)[2] * ((x - SCREEN_WIDTH / 2) * (*gameSettings)[0]) / y;
			float mappedY = (*camState)[2] * (*gameSettings)[1] / y;

            int rotatedX = input.cosRot * mappedX - input.sinRot * mappedY  + (*camState)[0];
            int rotatedY = input.sinRot * mappedX + input.cosRot * mappedY  + (*camState)[1];

            if (rotatedX > gameMaps[0]->w || rotatedX <= 0 || rotatedY > gameMaps[0]->h || rotatedY < 0){
                setPixel(screenSurface, x, y + SCREEN_HEIGHT / 2, SDL_MapRGB(pFormat, 0, 50, 0));
                continue;
            }

			Uint32 pixel = getPixel(gameMaps[0], rotatedX, rotatedY);
			setPixel(screenSurface, x, y + SCREEN_HEIGHT / 2, pixel);
		}
	}
	SDL_UpdateWindowSurface( window );
	}
}


int main( int argc, char * args[] )
{
	SDL_Window * gWindow = NULL;
    
	SDL_Surface * gScreenSurface = NULL; // surface for the window

	bool launchSuccess = init(&gWindow, &gScreenSurface);

	SDL_Surface * loadedMaps[4];

	loadedMaps[0] = SDL_LoadBMP("marioKartMap.bmp");

	float settings[8] = {1 ,180, -0.01, 1, 1, 0, 0, 0}; // cam scale x, cam scale y, rot speed, move speed
	float camState[4] = {0,0,500,0}; // x - horizontal, y - horizontal, fake "z" vertical  

		
	startLoop(gWindow, gScreenSurface,loadedMaps, &settings, &camState);

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
        else{
            *gScreenSurface = SDL_GetWindowSurface( *gWindow );
			printf("SDL WINDOW OK\n");
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

void applyInput(struct ORInput * in,  float (*camState)[4], float (*gameSettings)[8]){

    in->sinRot = sin((*camState)[3]);
    in->cosRot = cos((*camState)[3]);

    int moveX = in->xA * (*gameSettings)[3];
    int moveY = in->yA * (*gameSettings)[3];

    (*camState)[0] += in->cosRot * moveX - in->sinRot * moveY;
    (*camState)[1] += in->sinRot * moveX + in->cosRot * moveY;

    (*camState)[2] += in->zA * (*gameSettings)[3];
    (*camState)[3] += in->rot * (*gameSettings)[2];
}

void getInput(struct ORInput * inputOut,SDL_Event * event){
	Uint8* keystate = SDL_GetKeyboardState(NULL);
    (inputOut)->xA = 0;
    (inputOut)->zA = 0;
    (inputOut)->yA = 0;
    (inputOut)->rot = 0;
    (inputOut)->quit = false;

	if(keystate[SDL_SCANCODE_A] == 1){
        (inputOut)->xA -= 1;
    }
    if(keystate[SDL_SCANCODE_D]){
        (inputOut)->xA += 1;
    }
    if(keystate[SDL_SCANCODE_W]){
        (inputOut)->yA += 1;
    }
    if(keystate[SDL_SCANCODE_S]){
        (inputOut)->yA -= 1;
    }

    if(keystate[SDL_SCANCODE_LEFT] == 1){
        (inputOut)->rot -= 1;
    }
    if(keystate[SDL_SCANCODE_RIGHT]){
        (inputOut)->rot += 1;
    }
    if(keystate[SDL_SCANCODE_SPACE]){
        (inputOut)->zA += 1;
    }
    if(keystate[SDL_SCANCODE_LSHIFT]){
        (inputOut)->zA -= 1;
    }


    while(SDL_PollEvent(event)){
        switch ((*event).type){
            case SDL_QUIT:
                inputOut->quit = true;
            case SDL_KEYDOWN:
                if((*event).key.keysym.sym == SDLK_ESCAPE)
                    inputOut->quit = true; //quit
            break;
        }
    }
}
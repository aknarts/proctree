#include <math.h>
#include "toolkit.h"

struct texpair
{
    const char *mFilename;
    GLuint mHandle;
    int mClamp;
};

int gScreenWidth = 0;
int gScreenHeight = 0;
UIState gUIState = {0,0,0,0,0,0,0,0,0,0};
texpair * gTextureStore = NULL;
int gTextureStoreSize = 0;


void initvideo(int argc)
{
    const SDL_VideoInfo *info = NULL;
    int bpp = 0;
    int flags = 0;

    info = SDL_GetVideoInfo();

    if (!info) 
    {
        fprintf(stderr, "Video query failed: %s\n", SDL_GetError());
        SDL_Quit();
        exit(0);
    }

#ifdef _DEBUG
    int fsflag = 0;
#else
#ifdef FULLSCREEN_BY_DEFAULT
    int fsflag = 1;
#else
    int fsflag = 0;
#endif
#endif

    if (argc > 1) fsflag = !fsflag;

    if (fsflag) 
    {
        gScreenWidth = info->current_w;
        gScreenHeight = info->current_h;
        bpp = info->vfmt->BitsPerPixel;
        flags = SDL_OPENGL | SDL_FULLSCREEN;
    }
    else
    {
        if (argc == 0)
        {
            // window was resized
        }
        else
        {
            gScreenWidth = DESIRED_WINDOW_WIDTH;
            gScreenHeight = DESIRED_WINDOW_HEIGHT;
        }
        bpp = info->vfmt->BitsPerPixel;
        flags = SDL_OPENGL;
#ifdef RESIZABLE_WINDOW
        flags |= SDL_RESIZABLE;
#endif
    }

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    if (SDL_SetVideoMode(gScreenWidth, gScreenHeight, bpp, flags) == 0) 
    {
        fprintf( stderr, "Video mode set failed: %s\n", SDL_GetError());
        SDL_Quit();
        exit(0);
    }
   
#ifdef DESIRED_ASPECT
    float aspect = DESIRED_ASPECT;
    if (((float)gScreenWidth / gScreenHeight) > aspect)
    {
        float realx = gScreenHeight * aspect;
        float extrax = gScreenWidth - realx;

        glViewport( extrax / 2, 0, realx, gScreenHeight );
    }
    else
    {
        float realy = gScreenWidth / aspect;
        float extray = gScreenHeight - realy;

        glViewport( 0, extray / 2, gScreenWidth, realy );
    }
#else
    glViewport( 0, 0, gScreenWidth, gScreenHeight );
#endif

    reload_textures();    
}


static void do_loadtexture(const char * aFilename, int clamp = 1)
{
    int i, j;

    // Load texture using stb
	int x, y, n;
	unsigned char *data = stbi_load(aFilename, &x, &y, &n, 4);
    
    if (data == NULL)
        return;

    int l, w, h;
    w = x;
    h = y;
    l = 0;
    unsigned int * src = (unsigned int*)data;


    // mark all pixels with alpha = 0 to black
    for (i = 0; i < h; i++)
    {
        for (j = 0; j < w; j++)
        {
            if ((src[i * w + j] & 0xff000000) == 0)
                src[i * w + j] = 0;
        }
    }

    // Tell OpenGL to read the texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)src);
	glGenerateMipmap(GL_TEXTURE_2D);

    // and cleanup.
	stbi_image_free(data);

    if (clamp)
    {
        // Set up texture parameters
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    }
    else
    {
        // Set up texture parameters
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
}

char * mystrdup(const char *aString)
{
	int len = strlen(aString);
	char * d = new char[len+1];
	memcpy(d, aString, len);
	d[len] = 0;
	return d;
}

GLuint load_texture(char * aFilename, int clamp)
{
    // First check if we have loaded this texture already
    int i;
    for (i = 0; i < gTextureStoreSize; i++)
    {
        if (stricmp(gTextureStore[i].mFilename, aFilename) == 0)
            return gTextureStore[i].mHandle;
    }

    // Create OpenGL texture handle and bind it to use

    GLuint texname;
    glGenTextures(1,&texname);
    glBindTexture(GL_TEXTURE_2D,texname);

    do_loadtexture(aFilename, clamp);

    gTextureStoreSize++;

	texpair * t = (texpair *)realloc(gTextureStore, sizeof(texpair) * gTextureStoreSize);
	if (t != NULL)
	{
	    gTextureStore = t;
		gTextureStore[gTextureStoreSize-1].mFilename = mystrdup(aFilename);
		gTextureStore[gTextureStoreSize-1].mHandle = texname;
		gTextureStore[gTextureStoreSize-1].mClamp = clamp;
	}

    return texname;
}

extern void progress();
void reload_textures()
{
    // bind the textures to the same texture names as the last time.
    int i;
    for (i = 0; i < gTextureStoreSize; i++)
    {
        glBindTexture(GL_TEXTURE_2D, gTextureStore[i].mHandle);
        do_loadtexture(gTextureStore[i].mFilename, gTextureStore[i].mClamp);
		progress();
    }
}


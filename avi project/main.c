#include <stdio.h>
#include <stdlib.h>
#include <mmintrin.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>

//#include <pthread.h>


//Funcion auxiliar para crear el puntero a las imagenes / Aux function for create pointers to images.
inline uchar* getXY(IplImage* sc,int ejeX, int ejeY){
    return (uchar*) (sc->imageData + ((ejeX*sc->nChannels) + (sc->widthStep*ejeY)));
}
//Esta Función simplemente dibuja un cuadro señalando el objeto buscado a partir de sus coordenadas X, y
//This function just draw a square around the goal image from this coordinates
void PintaCuadrado(IplImage *Mariquita, IplImage *Frame,  int fila, int columna, int xorigen, int yorigen, int xant, int yant, int contador)
{


    //--------------------------------Dibujar cuadro / Draw square:
    int imageWidth  = Mariquita->width; //Ancho de imagen / wide
    int imageHeight = Mariquita->height; //Alto / Heigh
    int ImgWidth  = (int)(imageWidth);
    int ImgHeight = (int)(imageHeight);
    // Calculo distancia para pintar el cuadro desde el origen hasta tamaño
	// calculate distace for draw square from origin until size.
    int dw = (int) (imageWidth-ImgWidth)/2;
    int dh = (int) (imageHeight-ImgHeight)/2;
    //compruebo las coordenadas en cada pasada
	//check coordinates in every frame 
    int cordenadaAncho =columna, cordenadaAlto = fila;

    int desplazax, desplazay;
    int xan=0, yan=0, xact, yact;

    //calculo el desplazamiento
	//calculate displacement
    desplazax = xorigen - fila;
    desplazay = yorigen - columna;

    //desplazamiento actual:
	//current displacement
    xact= fila - xant;
    yact= columna - yant;


    //Muestro la posicion del frame inicial el original y su desplazamiento..
	//Show current frame, original and displacement.
    printf("\n Coordenadas Originales: Fila: %d  Columna: %d ------ ", yorigen, xorigen );
    printf("\n Coordenadas Actuales frame: %d son : Fila: %d Columna: %d", contador, cordenadaAncho, cordenadaAlto);

    printf("\n La imagen se a desplazado una distancia y: %d x: %d", desplazay, desplazax);

    if(contador>1)
     printf("\n Desplazamineto anterior x: %d  y: %d \n", xact, yact);

    //Dibujo el cuadrado, resto 20 porque son los pixeles aproximados para encuadrar bien el dibujo.
	//Draw the square, subtract 20 because are the approximate pixels to correctly surround the image.
	
    //También podria hacerlo con variables directamente calculando desde la col 0 hasta la posicion x(Pero esto es mas rapido)
	//Of course i can do it too using variable and calculate from column 0 to x.(But this is faster)
    cvRectangle(Frame,
                cvPoint(cordenadaAlto+ImgWidth, cordenadaAncho+ImgHeight),
                cvPoint(cordenadaAlto, cordenadaAncho),
                cvScalar(255, 0, 0, 0),
                1, 8, 0);

    //--------------------------fin dibujar rectangulo / end draw rectangle.



}
int calcularDiferencia(IplImage* Fondo, IplImage* Mariquita, int ejex, int ejey)
{

//variables:
int x,y,suma=0;
__m128i* ptrmariquita, *ptrfondo;

if(ejex+Mariquita->width > Fondo->width||ejey+Mariquita->height > Fondo->height) return -1;

for(y=0;y<Mariquita->height;y++)
{

    ptrfondo = (__m128i*) getXY(Fondo,ejex,ejey+y);
    ptrmariquita = (__m128i*) getXY(Mariquita,0,y);

    for(x=0;x<Mariquita->width*3;x+=16)
    {

        __m128i A = _mm_loadu_si128(ptrmariquita);
        __m128i B = _mm_loadu_si128(ptrfondo);

        __m128i S = _mm_sad_epu8(A,B);

        int *t= (int*)&S;

        suma+= t[0]+t[2];

        ptrmariquita++;
        ptrfondo++;
    }
}

return suma;
}

void Fmascara(IplImage* Mariquita)
{
    int x=0,y=0;

    for(y=0;y<Mariquita->height;y++)
    {
         unsigned char *Pm =(unsigned char *) Mariquita->imageData + y * Mariquita->widthStep; //Creo un puntero a mariquita

        for(x=0;x<Mariquita->width;x++)
        {
            if(*Pm!=255&&*(Pm+1)!=255&&*(Pm+2)!=255)
            {
                *Pm=0;
                *(Pm+1)=0;
                *(Pm+2)=0;
            }
            Pm+=3;
        }
    }
}

int getDiferencia(IplImage* Fondo, IplImage* Mariquita, IplImage* Mascara,int offsetX, int offsetY){
    int x,y,sum;
    IplImage* temp = cvCloneImage(Fondo);

    for(y=0;y<Mariquita->height;y++){
        unsigned char *Pm =(unsigned char *) Mascara->imageData + y * Mascara->widthStep;
        unsigned char *Pf =(unsigned char *) temp->imageData + ((offsetX*3) + (y+offsetY) * temp->widthStep);
        for(x=0;x<Mariquita->width;x++){
            *Pf = *Pf | *Pm;
            Pf++;Pm++;
            *Pf = *Pf | *Pm;
            Pf++;Pm++;
            *Pf = *Pf | *Pm;
            Pf++;Pm++;
        }
    }
    sum = calcularDiferencia(temp,Mariquita,offsetX,offsetY);



    cvReleaseImage(&temp);
    return sum;
}

int main(int argc, char** argv) {
    //coordenadas para posicion del frame1
	// coordinates for first frame.
    int x,y,dif,cont=1;
    int xant=0,yant=0;
    int fila, col, coordenada=0, coordenadafil=0;


    IplImage* Mariquita = cvLoadImage("M.bmp", 1); //Mariquita es la imagen móvil / Ladybug is the mobile image
    IplImage* Mascara= cvLoadImage("M.bmp", 1); //voy usarlo para la mascara de la mariquita / this will be used for ladybug mask

    CvCapture* VideoFondo = cvCaptureFromFile("Fondo.avi"); //Guardo el video completo aquí / Store video here

    IplImage* Frame = cvQueryFrame(VideoFondo); //Cada Frame se guarda aquí, cada vez que lo invocamos salta al siguiente. 
												//every frame is stored here every call jumps to next frame.

    unsigned char * origen;     //Este puntero lo uso para apuntar al primer píxel de color de la imagen Objetivo.
								//this pointer is for point at the first pixel of the goal image.


    Fmascara(Mascara); //Crear mascara de mariquita / create ladybug mask.

    //Posicion inicial del objeto /start position of goal image.
    x=295;
    y=216;

    xant=x;
    yant=y;

    dif= getDiferencia(Frame,Mariquita,Mascara,x,y);
    // coordenadas iniciales centro: x 295 - y 216
	// Initial center coordinates.

    printf("La diferencia es %d ",dif);

       PintaCuadrado(Mariquita, Frame, x, y, 295 , 216, xant, yant, cont);

    cvNamedWindow("Ventana1", 1);
    cvShowImage("Ventana1", Frame);
    cvWaitKey(100);

        x=x-5;
        y=y-5;

    while(Frame=cvQueryFrame(VideoFondo))
    {
        for(x=xant-10;x<xant+10;x++)
        {
            for(y=yant-10;y<yant+10;y++)
            {
                if(dif=getDiferencia(Frame,Mariquita,Mascara,x,y)<10000)
                {
                    cont++; //para numerar frames / to give  a numeric value to every frame.
                    xant=x; //Coordenadas actuales. las guardo como anteriores /actual coordinates, i save it like previus.
                    yant=y;

                    PintaCuadrado(Mariquita, Frame, x, y, 295 , 216, xant, yant, cont);

                        cvNamedWindow("Ventana1", 1);
                        cvShowImage("Ventana1", Frame);
                        cvWaitKey(10);
                        y=yant+10;
                        x=xant+10;
                }
            }
        }
    }


//liberamos memoria y destruimos la ventana.
//clean memory and destroy window.	  
    cvWaitKey(0);
    cvReleaseImage(&Frame);
    cvDestroyWindow("Ventana1");




    return EXIT_SUCCESS;
}



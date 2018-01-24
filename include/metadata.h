/*
 * OPERATING SYSTEMS DESING - 16/17
 *
 * @file 	metadata.h
 * @brief 	Definition of the structures and data types of the file system.
 * @date	01/03/2017
 */
#include <stdint.h>
#define MAX_FILE 64 				// Número máximo de ficheros que puede gestionar el sistema

typedef struct{
	uint8_t numInodos;
}SuperBloque;			// Esctructura superbloque. Sólo almacena el número de Inodos del sistema de ficheros.


typedef struct{
	char nombre[32];	// Nombre del fichero
	uint16_t tamanyo;	// Tamaño del fichero
	uint16_t CRCdatos;	// CRC del bloque de datos que identifica el iNodo
}Inodo;				// Estrcutura Inodo. Cada fichero tiene asociado un Inodo que almacena información sobre él.

/* Declaración de las variables */
SuperBloque s_bloque;
char mapaInodos[64];		// Mapa de Inodos. Indica qué Inodos están siendo utilizados. Cada posición del array toma valor 0 (Inodo libre) o 1(Inodo ocupado).
Inodo* ArrayInodos;		// Array de estructuras Inodo.
uint16_t CRCmetadata;		// CRC de los metadatos para comprobaciones de integridad.


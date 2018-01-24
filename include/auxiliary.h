/*
 * OPERATING SYSTEMS DESING - 16/17
 *
 * @file 	auxiliary.h
 * @brief 	Headers for the auxiliary functions required by filesystem.c.
 * @date	01/03/2017
 */

typedef struct{
	int estado;     // Estado del descriptor. 1 está en uso y 0 no lo está.
	int idFichero;  // Identificador del fichero. Se corresponde con la posición del Inodo en el vector de Inodos.
	int posicion;   // Puntero de posición del fichero.
}Descriptor;		// Estructura de descriptores. Sirve para saber que ficheros están abiertos y su puntero de posición.

Descriptor* ArrayDescriptores;	// Conjunto de descriptores utilizados
int iNodosPrimerBloque;		// Número de Inodos en el primer bloque de disco.
int iNodosExtra;		// Número de Inodos en el segundo bloque de disco.

int findFilebyName(char *fileName); // Busca un fichero en el disco por su nombre, si lo encuentra devuelve su identificador, si no devuelve -1.
int isOpen(int idFile); 	// Dice si el fichero con identificador idFile está abierto. Devuelve 1 si está abierto y 0 si está cerrado.
int firstFreeDesc(); 		// Devuelve el primer descriptor libre. Devuelve -1 si no hay ninguno libre.
int firstFreeInode();  		// Devuelve el identificador del primer Inodo libre. Devuelve -1 si no hay ningún inodo libre.
int writeMetadata(); 		// Escribe los metadatos de memoria al disco. Devuelve 0 si se ejecuta con éxito, -1 si se produce algún error.
int findDescFile(int idFile);	// Busca el descriptor asociado a un fichero. Devuelve el descriptor de un fichero con identificador idFile. Si no lo encuentra devuelve -1.
int getNumBloque(int idFile);   // Busca el número de bloque en el que se encuentra un fichero. Devuelve el número de bloque de datos correspondiente al fichero con identificador idFile. Si no lo encuentra  devuelve -1.
int updateCRCMetadata();	// Actualiza el valor del CRC de los metadatos. Devuelve -1 si se produce error y 0 si se ejecuta con éxito.

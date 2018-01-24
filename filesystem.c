/*
 * OPERATING SYSTEMS DESING - 16/17
 *
 * @file 	filesystem.c
 * @brief 	Implementation of the core file system funcionalities and auxiliary functions.
 * @date	01/03/2017
 */

#include "include/filesystem.h"		// Headers for the core functionality
#include "include/auxiliary.h"		// Headers for auxiliary functions
#include "include/metadata.h"		// Type and structure declaration of the file system
#include "include/crc.h"			// Headers for the CRC functionality
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/*
 * @brief 	Generates the proper file system structure in a storage device, as designed by the student.
 * @return 	0 if success, -1 otherwise.
 */
int mkFS(long deviceSize)
{
	long maxCapacidad= BLOCK_SIZE*(2+MAX_FILE);		// Capacidad máxima que puede gestionar el sistema de ficheros (2 bloques para metadatos y 64 bloques para datos).
	int minCapacidad= BLOCK_SIZE*2;				// Capacidad mínima que ha de tener el dispositivo para soportar el sistema de ficheros (1 bloque para metadatos y 1 bloque de datos)
	int numBloquesDatos;					// Número de bloques de datos.
	FILE *fp;						// Puntero a la primera posición de memoria del disco.
	int tamanyoDisco;					// Tamaño del disco sobre el que se desea formatear una partición.
	int tamPrimerBloqueOcupado;				// Número de bytes del primer bloque ocupados (sin considerar los inodos).

	/* Se comprueba que la partición a formatear no exceda el tamaño del disco */
	fp=fopen(DEVICE_IMAGE,"r");
	fseek(fp, 0L, SEEK_END);
	tamanyoDisco= ftell(fp);

	if(deviceSize>tamanyoDisco){
		return -1;
	}

	/* Se comprueba que la partición dispone de suficiente espacio para soportar el sistema de ficheros */
	if(deviceSize<minCapacidad){
		return -1;
	}	

	/* Se comprueba que el tamaño de la partición a formatear no exceda el tamaño que puede gestionar el sistema de ficheros. 
	   En caso de superarlo, se reduce la partición a formatear a la máxima capacidad que puede gestionar el sistema para no desperdiciar memoria del disco. */
	if(deviceSize>maxCapacidad){
		deviceSize=maxCapacidad;
	}

	/* Se calcula si es necesario añadir un bloque extra para los Inodos ya que puede ocurrir que no quepan en el primer bloque del disco*/
	numBloquesDatos= (deviceSize/2048)-1;
	tamPrimerBloqueOcupado= sizeof(s_bloque)+sizeof(mapaInodos)+sizeof(CRCmetadata); 
	iNodosPrimerBloque= (int) ((BLOCK_SIZE - tamPrimerBloqueOcupado)/sizeof(Inodo));	// Número de Inodos que caben en el primer bloque.
	iNodosExtra= numBloquesDatos-iNodosPrimerBloque;					// Número de Inodos que no caben en el primer bloque.
	if(iNodosExtra>0){									// Si no caben los Inodos en un sólo bloque, se reduce el número de bloques
		numBloquesDatos--;								// de datos en 1 (porque se necesita un bloque extra del disco para los Inodos)
		iNodosExtra= numBloquesDatos-iNodosPrimerBloque;				// y se actualiza el número de Inodos que ocuparán el segundo bloque de disco.
	}
	else{											// Si caben todos los Inodos en un bloque, se actualiza el número de Inodos extra
		iNodosExtra=0;									// a 0 y el número de Inodos en el primer bloque se actualiza al número de bloques de datos.
		iNodosPrimerBloque= numBloquesDatos;
	}

	/* Inicialización del superbloque */
	s_bloque.numInodos= numBloquesDatos;
	
	/* Inicialización del mapa de Inodos */
	memset(mapaInodos,0, MAX_FILE);

	/* Inicialización de los Inodos */					
	ArrayInodos= (Inodo *) calloc(s_bloque.numInodos, sizeof(Inodo));

	/* Escritura de los metadatos por defecto al disco*/
	if(writeMetadata()<0){
		return -1;
	}

	/* Liberación de la memoria reservada */
	free(ArrayInodos);

	return 0;
}

/*
 * @brief 	Mounts a file system in the simulated device.
 * @return 	0 if success, -1 otherwise.
 */


int mountFS(void)
{
	/* Lectura del primer bloque de disco para obtener los metadatos */
	char* r_bloque= (char *) malloc(BLOCK_SIZE);
	if(bread(DEVICE_IMAGE, 0, r_bloque)<0){
		return -1;
	}
	
	/* Traspaso del superbloque, mapa de Inodos y CRC a las variables del programa */
	memcpy(&s_bloque, r_bloque, sizeof(s_bloque));
	memcpy(mapaInodos, r_bloque + sizeof(s_bloque), sizeof(mapaInodos));
	memcpy(&CRCmetadata, r_bloque + sizeof(s_bloque) + sizeof(mapaInodos), sizeof(CRCmetadata));
	ArrayInodos= (Inodo *) calloc(s_bloque.numInodos, sizeof(Inodo));

	/* Cálculo del número de Inodos que ocupan el primer bloque de disco y el segundo bloque. 
	   Aunque este cálculo ya se realiza en mkfs se tiene que repetir aquí ya que esta es la primera función
	   del sistema de ficheros que ejecuta un programa cliente y la única forma
	   de disponer de estas variables en memoria es volver a calcularlas a partir de la información en disco.*/
	int tamanyoLibre= BLOCK_SIZE - sizeof(s_bloque) - sizeof(mapaInodos) - sizeof(CRCmetadata);	// Tamaño disponible en el primer bloque sin contar los Inodos.
	iNodosPrimerBloque= (int) (tamanyoLibre/sizeof(Inodo));						// Número de Inodos que caben en el primer bloque
	if(s_bloque.numInodos>iNodosPrimerBloque){							// Si el número de Inodos del sistema supera el número de Inodos que caben
		iNodosExtra=s_bloque.numInodos-iNodosPrimerBloque;					// en el primer bloque, entonces se necesita un segundo bloque para Inodos, de lo contrario no.
	}
	else{
		iNodosExtra=0;
		iNodosPrimerBloque=s_bloque.numInodos;
	}

	/* Traspaso de los Inodos del primer bloque de disco al vector de Inodos del programa */
	memcpy(ArrayInodos, r_bloque+sizeof(s_bloque)+sizeof(mapaInodos)+sizeof(CRCmetadata), sizeof(Inodo)*iNodosPrimerBloque);

	/* Lectura del segundo bloque de disco y traspaso de los Inodos de dicho bloque al vector de Inodos del programa */
	if(iNodosExtra>0){
		if(bread(DEVICE_IMAGE, 1, r_bloque)<0){
			return -1;
		}
		memcpy(ArrayInodos+sizeof(Inodo)*iNodosPrimerBloque, r_bloque, sizeof(Inodo)*iNodosExtra );
	}
	
	/* Liberación de la memoria reservada para la lectura del disco */
	free(r_bloque);

	/* Inicialización del array de descriptores */
	ArrayDescriptores= (Descriptor *) calloc(s_bloque.numInodos, sizeof(Descriptor));
	int i;
	for(i=0; i<s_bloque.numInodos; i++){
		ArrayDescriptores[i].idFichero=-1;	// No se puede poner a "0" ya que el identificador del fichero puede ser "0" y no habría forma de diferenciar
	}						// entre el identificador o si está inicializado.
	
	/* Comprobación de la integridad de los metadatos */
	if(checkFS()<0){
		return -1;
	}
	
	return 0;
}

/*
 * @brief 	Unmounts the file system from the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int unmountFS(void)
{
	/* Comprueba que no existen ficheros abiertos */
	int i;
	for(i=0; i<s_bloque.numInodos; i++){
		if(ArrayDescriptores[i].estado){
			return -1;
		}
	}

	/* Escribe los metadatos a disco */
	if(writeMetadata()<0){
		return -1;
	}

	/* Liberación de las variables utilizadas por el sistema de ficheros */
	free(ArrayInodos);
	free(ArrayDescriptores);
	iNodosPrimerBloque=0;
	iNodosExtra=0;
	CRCmetadata=0;
	memset(mapaInodos, 0, sizeof(mapaInodos));
	memset(&s_bloque, 0, sizeof(s_bloque));

	return 0;
}

/*
 * @brief	Creates a new file, provided it it doesn't exist in the file system.
 * @return	0 if success, -1 if the file already exists, -2 in case of error.
 */
int createFile(char *fileName)
{
	/* Comprobación de que no existe un fichero con el mismo nombre */
	if(findFilebyName(fileName)!=-1){
		return -1;
	}

	/* Comprobación de que el fichero tiene espacio en el disco */
	int iNodo_libre= firstFreeInode(); 
	if(iNodo_libre<0){
		return -2;
	}

	/* Comprobación de la longitud del nombre, si supera los 32 caracteres devuelve error */
	if(strlen(fileName)>32){
		return -2;
	}

	/* Creación del nuevo Inodo en el sistema de ficheros */
	strcpy(ArrayInodos[iNodo_libre].nombre, fileName);			
	ArrayInodos[iNodo_libre].tamanyo= 0;					
	ArrayInodos[iNodo_libre].CRCdatos= 0;					

	/* Formateo del bloque de datos asociado al Inodo creado */
	char* b_vacio= (char*) calloc(1, BLOCK_SIZE);
	int numBloque = getNumBloque(iNodo_libre);
	if(bwrite(DEVICE_IMAGE, numBloque, b_vacio)<0){
		return -2;
	}

	/* Actualización del valor del CRC del bloque de datos asociado al Inodo */
	ArrayInodos[iNodo_libre].CRCdatos= CRC16((unsigned char*)b_vacio, BLOCK_SIZE);

	/* Escritura de los metadatos a disco para que al abrir el fichero y comprobar su integridad no de fallo */
	if(writeMetadata()<0){
		return -2;
	}

	/* Liberación de la memoria reservada para escribir el bloque en disco */
	free(b_vacio);

	/* Modificación de los mapas */
	mapaInodos[iNodo_libre]=1;
	
	return 0;
}


/*
 * @brief	Deletes a file, provided it exists in the file system.
 * @return	0 if success, -1 if the file does not exist, -2 in case of error..
 */
int removeFile(char *fileName)
{
	/* Comprueba que existe un fichero con ese mismo nombre */
	int idFile= findFilebyName(fileName);
	if(idFile<0){
		return -1;
	}

	/* Comprueba que el fichero no esté abierto */
	if(isOpen(idFile)){
		return -2;
	}

	/* Modificación del mapa de Inodos */
	mapaInodos[idFile]=0;

	/* Borrado del iNodo del array de INodos */
	memset(&(ArrayInodos[idFile]),0,sizeof(Inodo)); 

	return 0;
}


/*
 * @brief	Opens an existing file.
 * @return	The file descriptor if possible, -1 if file does not exist, -2 in case of error..
 */
int openFile(char *fileName)
{
	/* Busca el fichero que se quiere abrir */
	int idFile= findFilebyName(fileName);

	/* Comprueba si existe el fichero */
	if(idFile<0){
		return -1;
	}

	/* Comprueba si el archivo ya está abierto */
	if(isOpen(idFile)){
		return -2;
	}
	
	/* Comprobación de la integridad del bloque de datos del fichero */ 
	if(checkFile(fileName)<0){
		return -2;
	}

	/* Comprueba que exista un descriptor sin usar */
	int descriptor=firstFreeDesc();
	if(descriptor<0){
		return -2;
	}

	/* Asigna al fichero el primer descriptor que no esté siendo usado */
	ArrayDescriptores[descriptor].estado=1;
	ArrayDescriptores[descriptor].idFichero=idFile;
	ArrayDescriptores[descriptor].posicion=0;

	/* Devuelve el descriptor asignado al fichero */
	return descriptor;
}

/*
 * @brief	Closes a file.
 * @return	0 if success, -1 otherwise.
 */
int closeFile(int fileDescriptor)
{
	/* Comprueba la validez de la entrada */
	if(fileDescriptor<0 || fileDescriptor>=s_bloque.numInodos){
		return -1;
	}

	/* Comprueba que el descriptor está siendo usado */
	if(!ArrayDescriptores[fileDescriptor].estado){
		return -1;
	}

	/* Obtención del número de bloque de datos del fichero que está utilizando el descriptor */
	int numBloque= getNumBloque(ArrayDescriptores[fileDescriptor].idFichero);

	/* Lectura del bloque de datos en el que se encuentra el fichero */
	char* r_bloque= (char*) calloc(1, BLOCK_SIZE);
	if(bread(DEVICE_IMAGE, numBloque, r_bloque)!=0){
		return -1;
	}
	
	/* Actualización del CRC del bloque de datos. El CRC es necesario que se actualice en esta función ya que en las operaciones de escritura no se actualiza.
	   El objetivo de esto es evitar que se estén escribiendo los metadatos cada vez que se modifica un fichero, de esta forma sólo se escriben
	   los metadatos al cerrarlo. */
	ArrayInodos[ArrayDescriptores[fileDescriptor].idFichero].CRCdatos= CRC16((unsigned char*)r_bloque, BLOCK_SIZE);
	if(writeMetadata()!=0){
		return -1;
	}

	/* Liberación de la memoria reservada para leer el fichero de disco */
	free(r_bloque);

	/* Liberación del descriptor */
	ArrayDescriptores[fileDescriptor].estado=0;
	ArrayDescriptores[fileDescriptor].idFichero=-1; 	//No se puede inicializar a 0 ya que se utiliza para identificar un fichero.
	ArrayDescriptores[fileDescriptor].posicion=0;

	return 0;
}

/*
 * @brief	Reads a number of bytes from a file and stores them in a buffer.
 * @return	Number of bytes properly read, -1 in case of error.
 */
int readFile(int fileDescriptor, void *buffer, int numBytes)
{
	/* Comprobación de la validez de las entradas */
	if(fileDescriptor<0|| fileDescriptor>=s_bloque.numInodos){
		return -1;
	}
	if(numBytes<=0){
		return -1;
	}

	/* Comprobación de que el descriptor está siendo utilizado por un fichero */
	if(!ArrayDescriptores[fileDescriptor].estado){
		return -1;
	}

	/* Obtención del identificador del fichero asociado al descriptor */
	int idFile= ArrayDescriptores[fileDescriptor].idFichero;

	/* Cálculo de la cantidad de bytes que se pueden leer del fichero */
	if(ArrayDescriptores[fileDescriptor].posicion+numBytes>ArrayInodos[idFile].tamanyo){
		numBytes= ArrayInodos[idFile].tamanyo - ArrayDescriptores[fileDescriptor].posicion; 
	}

	/* Si no se pueden leer bytes del fichero devuelve 0 */
	if(numBytes<=0){
		return 0;
	}

	/* Obtención del número de bloque en el que se encuentra el fichero a leer */
	int numBloque= getNumBloque(idFile);

	/* Lectura del bloque de datos en el que se encuentra el fichero a leer */
	char* b_aux= (char*) malloc(BLOCK_SIZE);
	if(bread(DEVICE_IMAGE, numBloque , b_aux)<0){
		return -1;
	}

	/* Copia de los primeros numBytes del buffer auxiliar al buffer de lectura */
	memmove(buffer, b_aux+ ArrayDescriptores[fileDescriptor].posicion, numBytes);

	/* Liberación de la memoria reservada */
	free(b_aux);

	/* Actualización del puntero de posición del fichero */
	ArrayDescriptores[fileDescriptor].posicion=ArrayDescriptores[fileDescriptor].posicion+numBytes;
	
	/* Devuelve el número de bytes leídos */
	return numBytes;
}

/*
 * @brief	Writes a number of bytes from a buffer and into a file.
 * @return	Number of bytes properly written, -1 in case of error.
 */
int writeFile(int fileDescriptor, void *buffer, int numBytes)
{
	/* Comprobación de la validez de las entradas */
	if(fileDescriptor<0 || fileDescriptor>=s_bloque.numInodos){
		return -1;
	}
	if(numBytes<=0){
		return -1;
	}

	/* Comprobación de que el descriptor tiene asociado un fichero */
	if(!ArrayDescriptores[fileDescriptor].estado){
		return -1;
	}
	
	/* Comprobación de la cantidad de bytes que se pueden escribir en el fichero */
	if(ArrayDescriptores[fileDescriptor].posicion+numBytes>BLOCK_SIZE){
		numBytes = BLOCK_SIZE - ArrayDescriptores[fileDescriptor].posicion;
	}
	/* Si no se pueden escribir más bytes en el fichero devuelve 0 */
	if(numBytes<=0){
		return 0;
	}

	/* Obtención del identificador del fichero asociado al descriptor */
	int idFile= ArrayDescriptores[fileDescriptor].idFichero;

	/* Obtención del número de bloque en el que se encuentra el fichero a escribir */
	int numBloque= getNumBloque(idFile);

	/* Lectura del bloque de datos en el que se encuentra el fichero sobre el que se quiere escribir */
	char* b_aux= (char*) malloc(BLOCK_SIZE);
	if(bread(DEVICE_IMAGE, numBloque , b_aux)<0){
		return -1;
	}

	/* Copia de los primeros numBytes del buffer de escritura sobre el buffer en el que se ha guardado el contenido
	   del fichero comenzando a escribir desde la posición indicada por el puntero de posición del fichero */
	memmove(b_aux+ ArrayDescriptores[fileDescriptor].posicion, buffer, numBytes);

	/* Escritura del fichero modificado a disco */
	if(bwrite(DEVICE_IMAGE, numBloque , b_aux)<0){
		return -1;
	}

	/* Liberación de la memoria reservada */
	free(b_aux);

	/* Actualización del puntero de posición del fichero */
	ArrayDescriptores[fileDescriptor].posicion=ArrayDescriptores[fileDescriptor].posicion+numBytes;

	/* Actualización del tamaño del fichero */
	if(ArrayDescriptores[fileDescriptor].posicion>ArrayInodos[idFile].tamanyo){
		ArrayInodos[idFile].tamanyo=ArrayDescriptores[fileDescriptor].posicion;
	}
	
	/* Devuelve el número de bytes escritos */
	return numBytes;
}

/*
 * @brief	Modifies the position of the seek pointer of a file.
 * @return	0 if succes, -1 otherwise.
 */
int lseekFile(int fileDescriptor, int whence, long offset)
{
	/* Comprobación de la validez de las entradas */
	if(fileDescriptor<0 || fileDescriptor>=s_bloque.numInodos){
		return -1;
	}

	/* Comprueba que haya un fichero asociado al descriptor */
	if(!ArrayDescriptores[fileDescriptor].estado){
		return -1;
	}

	/* Obtención del identificador del fichero asociado al descriptor */
	int idFile= ArrayDescriptores[fileDescriptor].idFichero;

	/* Comprueba que haya algo escrito en el fichero */
	if(!ArrayInodos[idFile].tamanyo){
		return -1;
	}

	/* El puntero se actualiza al principio del fichero */
	if(whence==FS_SEEK_BEGIN){
		ArrayDescriptores[fileDescriptor].posicion=0;
		return 0;
	}
	
	/* El puntero se actualiza al final del fichero */
	if(whence==FS_SEEK_END){
		ArrayDescriptores[fileDescriptor].posicion=ArrayInodos[idFile].tamanyo;
		return 0;
	}

	/* El puntero se situa en la posición actual y se desplaza un número bytes determinado por offset */
	if(whence==FS_SEEK_CUR){
	
		/* Comprobación de que se puede realizar el cambio de posición del puntero*/
		if(ArrayDescriptores[fileDescriptor].posicion+offset<0 || ArrayDescriptores[fileDescriptor].posicion+offset>ArrayInodos[idFile].tamanyo){
			return -1;
		}

		/* Actualización del puntero de posición del fichero */
		ArrayDescriptores[fileDescriptor].posicion=ArrayDescriptores[fileDescriptor].posicion+offset;
		return 0;
	}

	/* Si llega a este punto significa que el valor de whence es erróneo. Devuelve error. */
	return -1;
}

/*
 * @brief 	Verifies the integrity of the file system metadata.
 * @return 	0 if the file system is correct, -1 if the file system is corrupted, -2 in case of error.
 */
int checkFS(void)
{
	/* Comprueba que no haya ningún fichero abierto */
	int i;
	for(i=0; i<s_bloque.numInodos; i++){
		if(ArrayDescriptores[i].estado){
			return -2;
		}
	}

	/* Lectura del primer bloque de disco para obtener los metadatos */
	char* r_bloque= (char *) calloc(1, BLOCK_SIZE);
	if(bread(DEVICE_IMAGE, 0 , r_bloque)<0){
		return -2;
	}

	/* Obtención del valor de CRCMetadata guardado en el disco */
	uint16_t crcDisco;
	memcpy(&crcDisco, r_bloque + sizeof(s_bloque) + sizeof(mapaInodos) , sizeof(crcDisco));
	
	/* Copia de los metadatos guardados en disco a un buffer auxiliar. Puesto que el CRC se encuentra entre el mapa de Inodos y los Inodos
	   se tiene que copiar el contenido por partes para no copiar al buffer el CRC de los metadatos. */
	int numBytesMetadatos= sizeof(s_bloque) + sizeof(mapaInodos) + sizeof(Inodo)*s_bloque.numInodos;
	unsigned char* b_aux= (unsigned char *) calloc(1, numBytesMetadatos);
	memcpy(b_aux, r_bloque, sizeof(s_bloque) + sizeof(mapaInodos));
	memcpy(b_aux+sizeof(s_bloque) + sizeof(mapaInodos), r_bloque + sizeof(s_bloque) + sizeof(mapaInodos) + sizeof(CRCmetadata), iNodosPrimerBloque*sizeof(Inodo));
	if(iNodosExtra>0){
		if(bread(DEVICE_IMAGE, 1 , r_bloque)<0){
			return -2;
		}
		memcpy(b_aux + sizeof(s_bloque) + sizeof(mapaInodos) + iNodosPrimerBloque*sizeof(Inodo), r_bloque, iNodosExtra*sizeof(Inodo));
	}

	/* Aplicación de la función CRC a los metadatos obtenidos del disco */
	uint16_t crcMetadatos = CRC16(b_aux, numBytesMetadatos);

	/* Liberación de la memoria reservada */
	free(b_aux);
	free(r_bloque);

	/* Comparación entre el CRC obtenido del disco y el CRC calculado a partir de los metadatos de disco */
	if(crcDisco==crcMetadatos){
		return 0;
	}

	/* Si llega a este punto los metadatos están corruptos, devuelve error. */
	return -1;
}

/*
 * @brief 	Verifies the integrity of a file.
 * @return 	0 if the file is correct, -1 if the file is corrupted, -2 in case of error.
 */
int checkFile(char *fileName)
{
	/* Obtención del identificador del fichero con el nombre obtenido por parámetro */
	int idFile= findFilebyName(fileName);

	/* Comprueba que el fichero esté cerrado */
	if(isOpen(idFile)){
		return -2;
	}

	/* Copia de los Inodos del disco a memoria */
	char* r_bloque= (char *) calloc(1, BLOCK_SIZE);
	Inodo* vectorInodos= (Inodo *) calloc(1, sizeof(Inodo)*(iNodosPrimerBloque+iNodosExtra));
	if(bread(DEVICE_IMAGE, 0, r_bloque)<0){
		return -2;
	}
	memcpy(vectorInodos, r_bloque+sizeof(s_bloque)+sizeof(mapaInodos)+sizeof(CRCmetadata), sizeof(Inodo)*(iNodosPrimerBloque));
	
	if(iNodosExtra>0){
		if(bread(DEVICE_IMAGE, 1, r_bloque)<0){
			return -2;
		}
		memcpy(vectorInodos+sizeof(Inodo)*iNodosPrimerBloque, r_bloque, sizeof(Inodo)*iNodosExtra );
	}

	/* Obtención del Inodo que contiene la información del fichero con el nombre que se pasa por parámetro */
	uint16_t CRCbloqueDatos;
	int numBloqueDatos;
	int i;
	for(i=0; i<s_bloque.numInodos ; i++){
		/* Cuando encuentra el Inodo obtiene su CRC de bloque de datos y el número de bloque */
		if(!strcmp(fileName, vectorInodos[i].nombre)){
			CRCbloqueDatos=vectorInodos[i].CRCdatos;
			numBloqueDatos=getNumBloque(i);
		}
	}

	/* Lectura del bloque de datos del fichero */
	if(bread(DEVICE_IMAGE, numBloqueDatos, r_bloque)<0){
		return -2;
	}

	/* Obtención del CRC del bloque de datos del fichero */
	uint16_t CRCbloqueDatos2 = CRC16((unsigned char*)r_bloque, BLOCK_SIZE);

	/* Liberación de la memoria reservada */
	free(r_bloque);
	free(vectorInodos);

	/* Compara el valor de CRC obtenido del Inodo con el CRC calculado a partir del bloque de datos del fichero */
	if(CRCbloqueDatos==CRCbloqueDatos2){
		return 0;
	}	

	/* Si llega a este punto el fichero está corrupto, devuelve error. */
	return -1;
}

/*
 * @brief 	Busca en el sistema de ficheros un fichero con el nombre recibido por parámetro.
 * @return 	El id del fichero si lo encuentra, -1 si no encuentra un fichero con ese nombre.
 */
int findFilebyName(char *fileName){
	int i;
	for(i=0; i<s_bloque.numInodos ; i++){
		if(!strcmp(fileName, ArrayInodos[i].nombre)){
			return i;
		}
	}
	return -1;
}

/*
 * @brief 	Busca el primer Inodo libre de la lista de iNodos.
 * @return 	Devuelve el identificador del primer iNodo libre (si es que existe), -1 si no hay ninguno libre (el sistema de ficheros está lleno).
 */
int firstFreeInode(){
	int i;
	for(i=0; i<s_bloque.numInodos; i++){
		if(!mapaInodos[i]){
			return i;
		}
	}
	return -1;
}

/*
 * @brief 	Busca el primer descriptor libre de la lista de descriptores.
 * @return 	Devuelve el primer descriptor sin usar (si es que existe), -1 si no hay ningún descriptor sin usar.
 */
int firstFreeDesc(){
	int i;
	for(i=0;i<s_bloque.numInodos; i++){
		if(!ArrayDescriptores[i].estado){
			return i;
		}
	}
	return -1;
}

/*
 * @brief 	Comprueba si un fichero está abierto.
 * @return 	Devuelve 1 si el fichero recibido por parámetro está abierto, 0 si no lo está.
 */
int isOpen(int idFile){
	int i;
	for(i=0;i<s_bloque.numInodos; i++){
		if(idFile==ArrayDescriptores[i].idFichero){
			return 1;
		}
	}
	return 0;
}

/*
 * @brief 	Devuelve el descriptor asociado a un fichero.
 * @return 	Descriptor asociado al fichero con identificador idFile. -1 en caso de que ese fichero no tenga asociado un descriptor.
 */
int findDescFile(int idFile){
	int i;
	for(i=0;i<s_bloque.numInodos; i++){
		if(ArrayDescriptores[i].idFichero==idFile){
			return i;
		}
	}
	return -1;
}

/*
 * @brief 	Calcula el número de bloque correspondiente a un fichero con identificador idFile
 * @return 	Número de bloque de datos correspondiente al fichero con identificador idFile
 */
int getNumBloque(int idFile){
	if(!iNodosExtra){
		return idFile+1;
	}
	return idFile+2;
}

/*
 * @brief 	Escribe los metadatos a disco.
 * @return 	Si se ejecuta con éxito devuelve 0. Si se produce algún error devuelve -1. 
 */
int writeMetadata(){

	/* Actualiza el valor de CRC de metadatos */
	updateCRCMetadata();

	/* Escribe a disco los metadatos del primer bloque */
	char* w_bloque= calloc(1, BLOCK_SIZE);
	memcpy(w_bloque, &(s_bloque), sizeof(s_bloque));
	memcpy(w_bloque+sizeof(s_bloque), mapaInodos , sizeof(mapaInodos));
	memcpy(w_bloque+sizeof(s_bloque)+sizeof(mapaInodos), &CRCmetadata , sizeof(CRCmetadata));
	memcpy(w_bloque+sizeof(s_bloque)+sizeof(mapaInodos)+ sizeof(CRCmetadata), ArrayInodos , sizeof(Inodo)*iNodosPrimerBloque);
	if(bwrite(DEVICE_IMAGE, 0, w_bloque)<0){
		return -1;
	}
			
	/* Escribe a disco los metadatos del segundo bloque (si los hay) */
	if(iNodosExtra>0){
		memset(w_bloque, 0, BLOCK_SIZE);
		memcpy(w_bloque, ArrayInodos + sizeof(Inodo)*iNodosPrimerBloque, sizeof(Inodo)*iNodosExtra);
		if(bwrite(DEVICE_IMAGE, 1, w_bloque)!=0){
			return -1;
		}
	}

	/* Liberación de la memoria reservada */
	free(w_bloque);

	return 0;
}

/*
 * @brief 	Escribe los metadatos a disco.
 * @return 	Si se ejecuta con éxito devuelve 0. Si se produce algún error devuelve -1. 
 */
int updateCRCMetadata(){
	int numBytesMetadatos= sizeof(s_bloque) + sizeof(mapaInodos) + sizeof(Inodo)*s_bloque.numInodos;
	unsigned char* b_aux= (unsigned char *) calloc(1, numBytesMetadatos);
	memcpy(b_aux, &s_bloque, sizeof(s_bloque));
	memcpy(b_aux + sizeof(s_bloque), mapaInodos, sizeof(mapaInodos));
	memcpy(b_aux + sizeof(s_bloque) + sizeof(mapaInodos), ArrayInodos, sizeof(Inodo)*s_bloque.numInodos);
	CRCmetadata= CRC16(b_aux, numBytesMetadatos);
	return 0;
}

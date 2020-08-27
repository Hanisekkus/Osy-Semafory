#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <string.h>

int FORK_NUMBER = 2;


ssize_t readLine( int fd, void *buf, size_t count )
{

    static char localStorage[ 2048 ];
    static char overLoadStorage[ 2048 ];
    
    static int offset = 0;
    static int lenght = 0;
    static int overLoad = 0;

    int counterNewLine = 0;


    if( lenght - offset == 0 || overLoad )
    {

        offset = 0;
        lenght = read( fd, localStorage, 2048 );

    }
    
    for( int i = offset; i <= lenght; i ++ )
    {

        if( localStorage[ i ] == '\n' )
        {

            counterNewLine = i - offset + 1;
            break;

        }

        if( i == lenght )
        {

            memcpy( overLoadStorage, localStorage + offset, lenght - offset );
            overLoad = lenght - offset;

            return readLine( fd, buf, count );

        }

    }

    if( overLoad )
    {

        strncat( overLoadStorage, localStorage, overLoad );
        memcpy( buf, overLoadStorage, counterNewLine + overLoad );

        offset += counterNewLine;  
        counterNewLine += overLoad;
        overLoad = 0;
        return counterNewLine;

    }else
    {

        memcpy( buf, localStorage + offset, counterNewLine );
    
    }

    if( counterNewLine > count )
    {

        return -1;

    }

    offset += counterNewLine;  

    return counterNewLine;

}


int main( int num, char **args )
{

    setbuf( stdout, NULL );

    if( ! ( num <= 1 ) )
    {

        FORK_NUMBER = atoi( args[ 1 ] );

    }

    sem_t *sem_vyroba = sem_open( "/vyroba", O_RDWR | O_CREAT, 0600, 1 );
    sem_t *sem_spotreba = sem_open( "/spotreba", O_RDWR | O_CREAT, 0600, 0 );

    sem_init( sem_vyroba, 1, 1 );
    sem_init( sem_spotreba, 1, 0 );
    
    if( !sem_vyroba || !sem_spotreba )
    {

        printf( "Nejsou semafory.\n" );
        exit( -1 );

    }

    int count = 0;
    int multiplier = 0;
    int pipeSys[ 2 ];
     
    pipe( pipeSys );

    for( int i = 0; i < FORK_NUMBER; i++ )
    {// -- Vyroba potomku

        if( fork() == 0)
        {// -- Potomek

            while( 1 )
            {

                sem_wait( sem_vyroba );
                // -- Logika v semaforu pro potomka

                dprintf( pipeSys[ 1 ], "Potomek [%d]\n", getpid() );

                sem_post( sem_spotreba );

            }

        }

    }
    while( 1 )
    {// -- Rodic
     
        sem_wait( sem_spotreba );
        // -- Logika v semaforu pro rodice 

        char buf[ 256 ];
        int lenght = readLine( pipeSys[ 0 ], buf, sizeof( buf ) );
        
        count ++;

        dprintf( 1, "[row:%d][multiplier:%d] Rodic [%d] cte:", count, multiplier, getpid() );
        write( 1, buf, lenght );
        
        if( count % 10000 == 0 )
        {

            count = 0;
            multiplier ++;
            sleep( 1 );

        }
        sem_post( sem_vyroba );

    }

    return 0;

}
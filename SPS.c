// Tabulky na terminálu


//Vlastní funkce fgets z fgetc() zajistit dynamickou realokaci
// Načíst row a tu předělat na cells

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>

#define BASE_TABLE_SIZE 10
#define BASE_COL_SIZE 128 //Nepotřebný ?
#define BASE_COL_COUNT 5 // NEpotřebný !
#define BASE_ROW_SIZE 5
#define BASE_ROWSTR_SIZE 10

typedef struct Cols{
    int length;
    char *chars;
}Col;


typedef struct Rows{
    int length;
    Col *cols;
} Row;

typedef struct Tables{
    int length;
    Row *rows;
}Table;

bool isDel(char *str,int pos, char *del){ //Kontroluje zda-li je znak na pozici pos řetezce str oddělovač
    if(strchr(del,str[pos]) != 0){
        if( pos == 0 || str[pos-1] != '\\' ){ //Pokud oddělovač není uvozen zpětným lomítkem
            return true;
        }
    }
    return false;
}

void printTable(Table *table, char *del){ //Vytiskne na výstup jeden řádek tabulky
  //  printf("tisknu pičo\n");
    if(table->length > 0){
        for(int r = 0; r < table->length;r++){ // 
            for(int c = 0; c < table->rows[r].length; c++){ // 
                printf("(Cl#%d)",table->rows[r].cols[c].length);
                for(int s = 0; s < table->rows[r].cols[c].length;s++){
                    
                    if(table->rows[r].cols[c].chars[s] != '\\'){
                        printf("%c",table->rows[r].cols[c].chars[s]);
                    }
                }
                if( c < table->rows[r].length - 1){
                    printf("%c",del[0]);
                }
            }
            printf("\n");
          //  printf("\nkonec %s row#%d\n",table->rows[r].cols[0].chars,r);
        }
    }else{
        printf("kurva tak ne\n");
    }
}

void delChars(char *str,int from, int length) // Z libovolné pozice řetezce smaže několik znaků
{
   
  if((unsigned)(length+from) <= strlen(str))
  {
    for(int i = 0; i < length ; i++){
        memmove(&str[from],&str[from+1],strlen(&str)-from);
    }
  }else{
      printf("KURVA CHCEŠ TOHO MOC\n"); //DEBUG
  }
}


Row loadRow(char *rowStr, char *del){
 //   printf("load Row from : %s\n",rowStr);
     //TODO Pokud nějaký řádek obsahuje více buněk než jiný řádek tabulky, budou jiné řádky doplněny o prázdné buňky
    Row row;
    row.length = 0;
    row.cols = malloc(sizeof(Col));
    int colCounter = 0;
    int shift = 0;
    for(int i = 0; rowStr[i] != '\0';i++){
        if(isDel(rowStr,i,del)){ //Znak je delimiter -> jdu uložit buňku před ním 
            //if((colCounter % BASE_COL_COUNT == 0) && colCounter > 0){
            row.cols = realloc(row.cols, sizeof(Col) * (colCounter+1));
            Col col;
            col.chars=malloc(sizeof(char) * i + 1);
            col.length = i;
            strncpy(col.chars,rowStr,i);
       //     printf("#%d : %s\n",colCounter,col.chars);
            delChars(rowStr,0,i+1);
            row.cols[colCounter] = col;
            colCounter++;
            row.length++;
            i = 0;

        }
    }

    row.cols = realloc(row.cols, sizeof(Col) * (colCounter + 1)); //TODO Přidat kontroly

    row.cols[colCounter].chars = malloc( (strlen(rowStr) + 1 ) * sizeof(char));
    strncpy(row.cols[colCounter].chars,rowStr,strlen(rowStr));
    row.cols[colCounter].length = strlen(rowStr);
    row.length++;
    //Uložit poslední buňku

    /*
    row.length = 0;
    row.cols = malloc(sizeof(Col) * BASE_ROW_SIZE);
    //row.cols[0].chars = (char*) malloc(sizeof(char) * BASE_COL_SIZE);
    int count = 0; //Současná pozice v celém řádku
    int inColCount = 0; //Současná pozice v právě zpracovávaném sloupci
    int col = 0;   //Číslo právě zpracovávaného sloupce 
    for(int i=0; rowStr[i] != '\0';i++){
        char z = rowStr[i];
        if(z == '\t' || z == '\r' || z == '\f'){
            continue;
        }
    
        if(count % BASE_ROW_SIZE == 0){ //Zvětšení row.cols      //ZDE JE SEQFAULT / NĚJAKÁ CHYBA 
            Col *np;
            if(inColCount == 0 ){
                np = realloc(row.cols,sizeof(Col) * BASE_ROW_SIZE * 2);
            }else{
                np = realloc(row.cols,sizeof(Col) * count);
            } 
            printf("realloc done\n");
            if(np == NULL){  
                perror("Error : not enough memory"); //Nešlo alokovat
                row.length = -1;
                return row;
            }else{
                row.cols = np;
            }
        }
        if(inColCount % BASE_COL_SIZE == 0){ //zvětšní buňky
            char *np;
            if(inColCount == 0 ){
                np = realloc(row.cols[col].chars,sizeof(char) * BASE_COL_SIZE * 2);
            }else{
                np = realloc(row.cols[col].chars,sizeof(char) * inColCount);
            }   

            if(np == NULL){
                perror("Error : not enough memory"); //Nešlo alokovat
                row.cols[col].length = -1;
                return row;
            }else{
                row.cols[col].chars = np;
            }
        }
        if(strchr(del,z) != NULL || (z=='\n' && count > 1) ){ //Znak je delimiter
            row.cols[col].length = inColCount;
            row.cols[col].chars[inColCount] = '\0';
            col++;
            inColCount = 0;
            //printf("col : %s\n",row.cols[col - 1].chars);
            continue;
        }

        row.cols[col].chars[inColCount] = z; //Zapsání
        inColCount++;
        count++;
        
    }
    row.length = col;
    //printf("len : %d colCount : %d\n", row.length,row.cols->length);
    //printf("loadRow done %s \n",row.cols[row.length - 1].chars);*/

    return row;
}

void encodeStr(char *str, char *del){ 
    // Pro všechny buňky provede : 
    // Smaže uvozovky na začátku a konci  
    int rLoc = -1;
    int nLoc = -1;
    for(int i = 0; str[i] != '\0'; i++){ 
        if(str[i] == '\"'){
            if(i==0 || isDel(str,i-1,del) ){ //Před " je oddělovač (nebo začátek řádku)
                int nextDel = i + 1;
                while( !(isDel(str,nextDel,del)) && (str[nextDel] != '\0') && (str[nextDel] != '\n') && (str[nextDel] != '\r')){
                    nextDel++;
                }
                if(str[nextDel-1] == '\"'){ // Uvozovka je na začátku i na konci buňky
                    delChars(str,i,1); //smaže první uvozovku
                    delChars(str,nextDel-2,1); //Smaže druhou uvozovku
                }
            }
        }else if( str[i] == '\r'){
            rLoc = i;
        }else if( str[i] == '\n'){
            nLoc = i;
        }
    }
    if(rLoc != -1){
        delChars(str,rLoc,1); //TODO fix dis nemaže když má 
    }
    if(nLoc != -1){
        delChars(str,nLoc,1);
    }
}

Table loadTable(char *url,char *del){
   
    Table table;
    table.length = 0;
    table.rows = malloc(sizeof(Row));

    FILE *fp = fopen(url,"r");
    if(fp == NULL){ 
        perror("Soubor neexistuje."); 
        table.length = -1;
        return table; 
    }
    int counter = 1;
 //   printf("for rowStr allocated : %d bytes\n",sizeof(char) * BASE_ROWSTR_SIZE);
    char *rowStr = malloc(sizeof(char) * BASE_ROWSTR_SIZE);
    //Načítá po jednom řádku
    while(fgets(rowStr,BASE_ROWSTR_SIZE - 1,fp) != NULL){
        int reallocCount = 2;
      //  printf("Bylo zapsání %d bytes\n",sizeof(rowStr));

        while(rowStr[strlen(rowStr) - 1] != '\n' &&  feof(fp)==0){ //rowStr není dost velká pro načtení celého řádku
            //Zvětšení paměti pro řádek
         //   printf("wjile stgart\n");
            
            char *str2 = malloc(sizeof(char) * BASE_ROW_SIZE);
            fgets(str2,BASE_ROWSTR_SIZE,fp);
         //   printf("realloc for rowS\n");
         //   printf("Bylo rowStr reallocCount%d realokováno %d bytů\n",reallocCount,reallocCount * sizeof(char) * BASE_ROWSTR_SIZE);
            char *np = realloc(rowStr, reallocCount * sizeof(char) * BASE_ROWSTR_SIZE);
            reallocCount++;
            if(np == NULL){
                perror("Error");
                table.length = -1;
                return table;
            }else{
                rowStr = np;
            //    printf("Bylo zapsáno %d bytů celkme %d bytů\n",sizeof(str2),sizeof(rowStr) + sizeof(str2) + 1);
            //    printf("bf strcat %s\n",rowStr);
                strcat(rowStr,str2);
             //   printf("AFTER STRCAT l%d %s\n",sizeof(rowStr),rowStr);
            }
            free(str2);
         //   printf("tr\n"); 
          //  printf("whileend\n");
        }
   //     printf("Out of tableRealloc : %s\n",rowStr);
        encodeStr(rowStr,del);
 //       printf("load row\n");
        Row row = loadRow(rowStr,del);  
        if(row.length == -1){
            table.length = -1;
            return table;
        }

        //Zvětšení paměti pro tabulku o řádek

        Row *np = realloc(table.rows,counter * sizeof(Row));
        if(np == NULL){
            perror("Error");
            table.length = -1;
            return table;
        }else{
            table.rows = np;
        }

        table.length++;

        table.rows[counter - 1] = row;
        counter++;

        
    }    /*
    printf("LoadTABLE OUT :  \n");
        for(int r = 0; r < table.length;r++){
            printf("table.rows[%d] : ",r);
            for(int i = 0; i < table.rows->length;i++){
                printf("%s",table.rows[r].cols[i].chars);
            }
            printf("\n");
        }*/

    return table;
}


int main(int argc,char *argv[]){
    char del[22] = ": "; 
    if(argc>1){  // Načtení oddělovače
        for(int i = 1;i<argc;i++){ 
            if(strcmp(argv[i],"-d") == 0){ //Uživatel chce určit oddělovač
                int delIn = 0;
                for(int y = 0; argv[i+1][y] != '\0';y++){
                    del[delIn] = argv[i+1][y]; 
                    delIn++; 
                }
            }
        }
    }

    if(argc>=2){
        Table table = loadTable(argv[1],del);
        //Zarovnat na stejný počet sloupců
        /*
        for(int r = 0; r < table.length;r++){
            printf("r#%d:",r);  
            for(int i = 0; i < table.rows->length;i++){
                printf("%s",table.rows[r].cols[i].chars);
            }
            printf("\n");   
        }*/
        printf("r 3 c 5 %s\n",table.rows[2].cols[4].chars);
        if(table.length != -1){
            printTable(&table,del);
        }
    }
    return 0;
}
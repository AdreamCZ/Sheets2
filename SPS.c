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


void delChars(char *str,int from, int length) // Z libovolné pozice řetezce smaže několik znaků
{
   
  if((unsigned)(length+from) <= strlen(str))
  {
    for(int i = 0; i < length ; i++){
        memmove(&str[from],&str[from+1],strlen(str)-from);
    }
  }else{
      printf("KURVA CHCEŠ TOHO MOC\n"); //DEBUG
  }
}


bool isDel(char *str,int pos, char *del){ //Kontroluje zda-li je znak na pozici pos řetezce str oddělovač
    if(strchr(del,str[pos]) != 0){
        if( pos == 0 || str[pos-1] != '\\' ){ //Pokud oddělovač není uvozen zpětným lomítkem
            char *strD = strdup(str); //Nutná deuplikace protože budu mazat části
            int cnt = 0;
            while(strchr(strD,'\"') != NULL){ // TODO FIX pokud jsou více jak 2 uvozovky -> nekonečný cyklus
                int parPos = strchr(str,'\"') - str;
                if(parPos < pos ){  //Někde před oddělovačem je uvozovka
                    delChars(strD,0,parPos + 1); //V strD zůstane jen část řetězce za uvozovkou 
                    if(strchr(strD,'\"') != NULL){
                        int par2Pos = strchr(strD,'\"') - strD;
                        delChars(strD,0,par2Pos + 1);
                        if(par2Pos > pos){
                            return false;
                        }
                    }
                }else{
                    break;
                }
                cnt++;
                if(cnt > 5){
                    printf("strD %s\n",strD);
                }
            }
            return true;
            

        }
    }
    return false;
}

void freeTable(Table *tab){ //Uvolní pamět allokovanou pro tabulku //TODO somehow test dis
    for(int r = 0; r < tab->length;r++){
        for(int c = 0; c < tab->rows[r].length;c++){
            free(tab->rows[r].cols[c].chars);
        }
    }
    free(tab);
}

void printTable(Table *table, char *del){ //Vytiskne na výstup jeden řádek tabulky
    printf("tisknu pičo tab len%d\n",table->length);
    printf("row 2 len %d\n",table->rows[1].length);
    if(table->length > 0){
        for(int r = 0; r < table->length;r++){ // 
    //    printf("r %d\n",r);
           // printf("#%d cc%d",r,table->rows[r].length);
            for(int c = 0; c < table->rows[r].length; c++){ // 
                printf("(Cl#%d)",table->rows[r].cols[c].length);
                bool hasSpecial = false;
                if(strchr(table->rows[r].cols[c].chars,'\\') != NULL){
                    putc('\"',stdout);
                    hasSpecial = true;
                }
                for(int s = 0; s < table->rows[r].cols[c].length;s++){
                    if(table->rows[r].cols[c].chars[s] != '\\'){
                        printf("%c",table->rows[r].cols[c].chars[s]);
                    }
                }
                if(hasSpecial){
                    putc('\"',stdout);
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


Row loadRow(char *rowStr, char *del){
 //   printf("load Row from : %s\n",rowStr);
     //TODO Pokud nějaký řádek obsahuje více buněk než jiný řádek tabulky, budou jiné řádky doplněny o prázdné buňky
    Row row;
    row.length = 0;
    row.cols = malloc(sizeof(Col));
    int colCounter = 0;
    for(int i = 0; rowStr[i] != '\0';i++){ 
        if(isDel(rowStr,i,del)){ //Znak je delimiter -> jdu uložit buňku před ním 
            Col *np = realloc(row.cols, sizeof(Col) * (colCounter + 1)); //TODO Přidat kontroly
            if(np == NULL){
                perror("Error");
                row.length = -1;
                return row;
            }else{
                row.cols = np;
            }
            Col col;
            col.chars=malloc(sizeof(char) * i + 1);
            col.length = i;
            strncpy(col.chars,rowStr,i);
            col.chars[i] = '\0';
            delChars(rowStr,0,i+1);
            row.cols[colCounter] = col; //Vložení vytvořené buňky do řádku
            colCounter++;
            row.length++;
            i = 0;

        }
    }

    Col *np = realloc(row.cols, sizeof(Col) * (colCounter + 1)); //TODO Přidat kontroly
    if(np == NULL){
        free(row.cols);
        perror("Error");
        row.length = -1;
        return row;
    }else{
        row.cols = np;
    }

    //Uložení poslední buňky
    row.cols[colCounter].chars = malloc( (strlen(rowStr) + 1 ) * sizeof(char));
    strncpy(row.cols[colCounter].chars,rowStr,strlen(rowStr));
    row.cols[colCounter].length = strlen(rowStr);
    row.length++;
    
    return row;
}

void encodeStr(char *str, char *del){ //TODO Mezi uvozovkami je delimiter tak se nemá brát v potaz
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
            char *str2 = malloc(sizeof(char) * BASE_ROWSTR_SIZE);
            fgets(str2,BASE_ROWSTR_SIZE,fp); //Přečte zbytek řádku
            char *np = realloc(rowStr, reallocCount * sizeof(char) * BASE_ROWSTR_SIZE);
            reallocCount++;
            if(np == NULL){
                free(rowStr);
                perror("Error");
                table.length = -1;
                return table;
            }else{
                rowStr = np;
                strcat(rowStr,str2);
            }
            free(str2);
        }
  //      encodeStr(rowStr,del);
        for(int i = 0; rowStr[i] != '\0'; i++){ //Vymže znaky, které by mohli později způsobovat problémy
            if(rowStr[i] == '\n' || rowStr[i] == '\r' || rowStr[i] == '\t'){
                delChars(rowStr,i,1);
            }
        }
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
    printf("retrun table\n");
    return table;
}

void alignCols(Table *table){
    int maxCols = 0;
    for(int i = 0; i < table->length;i++){
        maxCols = table->rows[i].length > maxCols ? table->rows[i].length : maxCols; //Zjistí kolik sloupců má nejdelší řádek
    }
    printf("maxCols %d\n",maxCols);
    for(int r = 0; r < table->length;r++){
        if(table->rows[r].length < maxCols){
            table->rows[r].cols = realloc(table->rows[r].cols, (maxCols+1) * sizeof(Col)); //TODO Kontroly maxCols -1 ?
            for(int c = table->rows[r].length;c < maxCols;c++){    
                table->rows[r].cols[c].length = 0;
                table->rows[r].cols[c].chars = malloc( sizeof(char) );
                table->rows[r].cols[c].chars[0] = '\0';

            }
            table->rows[r].length = maxCols;
            
        }
    }
    printf("done align\n");
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
        alignCols(&table);
        printf("After len %d\n",table.rows[1].length);
        
        /*
        for(int r = 0; r < table.length;r++){
            printf("r#%d:",r);  
            for(int i = 0; i < table.rows->length;i++){
                printf("%s",table.rows[r].cols[i].chars);
            }
            printf("\n");   
        }*/
      
        if(table.length != -1){
            printTable(&table,del);
        }else{
            freeTable(&table);
            return 1;
        }
    }
    return 0;
}
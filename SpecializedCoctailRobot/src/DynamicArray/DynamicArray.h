#ifndef DynamicArray_h
#define DynamicArray_h

class DynamicArray
{
private:
    int *array;
    int N;

public:
    DynamicArray() {
        array = nullptr;
        N = 0;
    }
    int *getArray() {
        return this->array;
    }
    int getN() {
        return this->N;
    }
    void clearArray() {
        delete[] this->array;
        array = nullptr;
        this->N = 0;
    }
    void createArray(int *&array, int N) {
        array = new int[N];
    }
    void removeArray(int *&array) {
        delete[] array;
        array = nullptr;
    }
    void addValue(int value) {
        int *tempArray;
        createArray(tempArray, N + 1);
        for (int i = 0; i < N; i++)
        {
            tempArray[i] = array[i];
        }
        tempArray[N] = value;
        removeArray(array);
        N++;
        array = tempArray;
    }
    void invert(){
        int *tempArray;
        createArray(tempArray,N);
        for (int i = 0; i < N; i++)
        {
            tempArray[i] = array[N - i - 1];
        }
        removeArray(array);
        array = tempArray;
        printArray();
    }

    void printArray()
    {
        for (int i = 0; i < N; i++){
            Serial.print(i);
            Serial.print(" : ");
            Serial.println(array[i]);
        }
    }
};

#endif
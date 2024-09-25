#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
using namespace std;

template<typename T>
class DataSource {
public:
    virtual ~DataSource() = default;

    virtual T next() = 0;
    virtual T* next(size_t& count) = 0;
    virtual bool hasNext() const = 0;
    virtual bool reset() = 0;
    virtual DataSource<T>* clone() const = 0;
    T operator()()
    {
        return this->next();
    }
    DataSource<T>& operator>>(T& element)
    {
        element = this->next();
        return *this;
    }
    operator bool() const
    {
        return this->hasNext();
    }
};


template<typename T>
class DefaultDataSource : public DataSource<T> {
public:
    T next() override {
        return T{};
    }

    T* next(size_t& count) override {
        T* newNext = new T[count];

        try {
            for (size_t i = 0; i < count; i++)
                newNext[i] = next();
        }
        catch (...)
        {
            delete[] newNext;
            throw;
        }

        return newNext;
    }

    bool hasNext() const override {
        return true; 
    }

    bool reset() override {
        return true; 
    }

    virtual DataSource<T>* clone() const override
    {
        return new DefaultDataSource(*this);
    }
    //The compiler will automatically generate a destructor
};

template<typename T>
class FileDataSource : public DataSource<T> {
public:
    FileDataSource(const char* filename)  {
        if (!filename)
        {
            throw std::invalid_argument("Invalid filename");
        }
        
        this->filename = new char[strlen(filename) + 1];
        strcpy(this->filename, filename);

        file.open(this->filename);
        if (!file.is_open()) {
            delete[] this->filename;
            throw std::runtime_error("Unable to open file");
        }
    }

    FileDataSource(const FileDataSource& other)  {
        filename = new char[strlen(other.filename) + 1];
        strcpy(filename, other.filename);

        file.open(filename);
        if (!file.is_open()) {
            delete[] filename;
            throw std::runtime_error("Unable to open file in copy constructor");
        }
    }


    FileDataSource& operator=(const FileDataSource& other) {
        if (this != &other) {

            std::ifstream tempFile(other.filename); //assuring that the other file is good if not the object is left unchanged
            if (tempFile.is_open()) {
               
                char* newFileName = new char[strlen(other.filename) + 1];
                strcpy(newFileName, other.filename);

                
                delete[] filename;
                //file.close() no need for explicit call
                filename = newFileName;
                file = tempFile; 
            }
            else {
                std::cerr << "Warning: Unable to open file in assignment operator. No changes made." << std::endl;
            }
        }
        return *this;
    }


    FileDataSource(FileDataSource&& other) noexcept : file(std::move(other.file)), filename(other.filename){
        other.filename = nullptr;
    }

    FileDataSource& operator=(FileDataSource&& other) noexcept {
        if (this != &other) {
         
            std::swap(file, other.file);
            std::swap(filename, other.filename);
        }
        return *this;
    }


    virtual ~FileDataSource() override {
        delete[] filename;
    }

    virtual T next() override {
        if (file.is_open() && file.good()) {
            T value{};
            if (!(file >> value)) {
                if (file.eof()) {
                    throw std::runtime_error("Reached end of file.");
                }
                else {
                    throw std::runtime_error("Failed to read from file.");
                }
            }
            return value;
        }
        else
        {
            throw std::exception("Something wrong with the file");
        }
    }

    virtual T* next(size_t& count) override {
        
        if (file.is_open() && file.good()) {
            T* values = nullptr;
            size_t elementsRead = 0;

            try {
                values = new T[count];

                while (elementsRead < count && this->hasNext()) {
                    T value{};
                    file >> value;

                    if (!file) {
                        throw std::runtime_error("Failed to read from file.");
                    }


                    values[elementsRead] = value;
                    elementsRead++;
                }


                count = elementsRead;

            }
            catch (...) {
                delete[] values;
                throw;
            }

            return values;
        }
        else
        {
            throw std::exception("Something wrong with the file");
        }
    }

    virtual bool hasNext() const override {
        if (!file.is_open() || !file.good() || file.eof()) {
            return false;
        }

       /* int nextChar = file.get();
        if (file.eof()) {
            return false;
        }

        file.unget();*/
        //maybe another way of checking 100% if there is a next charecter
        //but i should remove the const 

        return file.good();
    }

    virtual bool reset() override {
        if (!file.is_open()) {
            return false;
        }

        file.clear();
        if (!file) {
            return false;
        }

        file.seekg(0, std::ios::beg);
        if (!file) {
            return false;
        }

        return true;
    }

    virtual DataSource<T>* clone() const override {
        return new FileDataSource(*this);
    }


private:
    std::ifstream file;
    char* filename;
};



template<typename T>
class ArrayDataSource : public DataSource<T> {
public:
    ArrayDataSource(const T* data, size_t size) : current(0), size(size)
    {
        if (!data)
        {
            throw std::invalid_argument("Data is nullptr");
        }

        if (size == 0)
            throw std::invalid_argument("Size cant be 0");
        this->data = new T[size];
        for (size_t i = 0; i < size; i++)
        {    
            try {
                this->data[i] = data[i];
            }
            catch (...)
            {
                delete[] this->data;
                throw;
            }
        }
    }
    ArrayDataSource(const ArrayDataSource& other)
    {
        this->data = new T[other.size];
        for (size_t i = 0; i < other.size; i++)
        {
            try {
                this->data[i] = other.data[i];
            }
            catch (...)
            {
                delete[] this->data;
                throw;
            }
        }

        this->size = other.size;
        this->current = other.current;
    }
    ArrayDataSource(ArrayDataSource&& other) noexcept : data(nullptr), current(0), size(0)
    {
        *this = std::move(other);
    }
    ArrayDataSource& operator=(const ArrayDataSource& other)
    {
        if (this != &other)
        {
            T* newData = new T[other.size];

            for (size_t i = 0; i < other.size; i++)
            {
                try {
                    newData[i] = other.data[i];
                }
                catch (...)
                {
                    delete[] newData;
                    throw;
                }
            }

            delete[] this->data;
            this->data = newData;
            this->size = other.size;
            this->current = other.current;
        }

        return *this;
    }
    ArrayDataSource& operator=(ArrayDataSource&& other) noexcept
    {
        if (this != &other) {
            std::swap(this->data, other.data);
            std::swap(this->size, other.size);
            std::swap(this->current, other.current);
        }
        return *this;
    }
    virtual T next() override {
        if (this->size <= this->current)
            throw std::invalid_argument("Index out of range");
        return this->data[this->current++];
    }

    virtual T* next(size_t& count) override {
        size_t available = size - current;
        size_t actualCount = (count < available) ? count : available;

        T* values = nullptr;
        try {
            values = new T[actualCount];

            for (size_t i = 0; i < actualCount; i++) {
                values[i] = this->data[current];
                current++;
            }
            count = actualCount;

        }
        catch (...) {
            delete[] values;  
            throw; 
        }

        return values;
    }


    virtual bool hasNext() const override {
        return this->current < this->size;
    }

    virtual bool reset() override {
        this->current = 0;
        return true;
    }

    ArrayDataSource<T>& operator+=(const T& element) {
        T* newData = new T[this->size + 1];
        for (size_t i = 0; i < this->size; i++) {
            try {
                newData[i] = this->data[i];
            }
            catch (...) {
                delete[] newData;
                throw;
            }
        }
        try {
            newData[this->size] = element;
        }
        catch (...)
        {
            delete[] newData;
            throw;
        }
        delete[] this->data;
        this->data = newData;
        this->size++;
        return *this;
    }

    ArrayDataSource<T>& operator--() {
        if (this->current > 0) {
            --this->current;
        }
        return *this;
    }

    ArrayDataSource<T>& operator--(int) {
        ArrayDataSource<T>& temp = *this;
        --(*this);
        return temp;
    }
    virtual ~ArrayDataSource() override
    {
        delete[] this->data;
    }

    virtual DataSource<T>* clone() const override
    {
        return new ArrayDataSource(*this);
    }

private:
    T* data;
    size_t current;
    size_t size;
};

template<typename T>
ArrayDataSource<T> operator+(const ArrayDataSource<T>& arr, const T& element) {
    ArrayDataSource<T> result(arr);
    result += element;
    return result;
}

template<typename T>
class AlternateDataSource : public DataSource<T> {
public:
    AlternateDataSource(DataSource<T>** sources, size_t sourceCount)
        :sourceCount(sourceCount), currentSource(0)
    {
        if (!sources)
        {
            throw std::invalid_argument("Sources is nullptr");
        }

        if (sourceCount == 0)
        {
            throw std::invalid_argument("Soruce count is 0");
        }
        this->sources = new DataSource<T>*[sourceCount];

        for (size_t i = 0; i < sourceCount; i++) {
            try {
                this->sources[i] = sources[i]->clone();
            }
            catch (...) {
                for (size_t j = 0; j < i; j++)
                    delete this->sources[j];
                delete[] this->sources;
                throw;
            }
        }
    }

    AlternateDataSource(const AlternateDataSource& other)
        : sources(nullptr), sourceCount(other.sourceCount), currentSource(other.currentSource)
    {
        this->sources = new DataSource<T>*[other.sourceCount];

        for (size_t i = 0; i < other.sourceCount; i++) {
            try {
                this->sources[i] = other.sources[i]->clone();
            }
            catch (...) {
                for (size_t j = 0; j < i; j++)
                    delete this->sources[j];
                delete[] this->sources;
                throw;
            }
        }
    }

    AlternateDataSource(AlternateDataSource&& other) noexcept
        : sources(other.sources), sourceCount(other.sourceCount), currentSource(other.currentSource)
    {
        other.sources = nullptr;
        other.sourceCount = 0;
        other.currentSource = 0;
    }

    AlternateDataSource& operator=(const AlternateDataSource& other)
    {
        if (this != &other) {
            DataSource<T>** newData = new DataSource<T>*[other.sourceCount];

            for (size_t i = 0; i < other.sourceCount; i++) {
                try {
                    newData[i] = other.sources[i]->clone();
                }
                catch (...) {
                    for (size_t j = 0; j < i; j++)
                        delete newData[j];
                    delete[] newData;
                    throw;
                }
            }

            for (size_t i = 0; i < this->sourceCount; i++) {
                delete this->sources[i];
            }
            delete[] this->sources;

            this->sources = newData;
            this->sourceCount = other.sourceCount;
            this->currentSource = other.currentSource;
        }

        return *this;
    }

    AlternateDataSource& operator=(AlternateDataSource&& other) noexcept
    {
        if (this != &other) {
            std::swap(this->sources, other.sources);
            std::swap(this->sourceCount, other.sourceCount);
            std::swap(this->currentSource, other.currentSource);
        }
        return *this;
    }

    virtual T next() override {
        if (sourceCount == 0) return T{};

        T value;
        size_t start = currentSource;

        do {
            if (*sources[currentSource]) {
                value = sources[currentSource]->next();
                currentSource = (currentSource + 1) % sourceCount;
                return value;
            }
            currentSource = (currentSource + 1) % sourceCount;
        } while (currentSource != start);

        return T{};
    }

    virtual T* next(size_t& count) override {
        if (count == 0)
            throw std::invalid_argument("Invalid count for next");

        T* values = new T[count];
        size_t elemtsRead{};
        for (size_t i = 0; i < count; i++) {
            if (this->hasNext())
            {
                try {
                    values[i] = this->next();
                    elemtsRead++;
                }
                catch (...)
                {
                    delete[] values;
                    throw;
                }
            }
            else
            {
                count = elemtsRead;
                return values;
                
            }
        }

        return values;
    }

    virtual bool hasNext() const override {
        for (size_t i = 0; i < sourceCount; i++) {
            if (sources[i]->hasNext()) return true;
        }
        return false;
    }

    virtual bool reset() override {
        for (size_t i = 0; i < sourceCount; i++) {
            if (!sources[i]->reset()) return false;
        }
        currentSource = 0;
        return true;
    }

    virtual DataSource<T>* clone() const override
    {
        return new AlternateDataSource(*this);
    }

    virtual ~AlternateDataSource() override
    {
        for (size_t i = 0; i < this->sourceCount; i++)
            delete this->sources[i];
        delete[] this->sources;
    }

private:
        DataSource<T>** sources;
        size_t sourceCount;
        size_t currentSource;

};

// Клас GeneratorDataSource
template<typename T, typename Generator>
class GeneratorDataSource : public DataSource<T> {
public:
    GeneratorDataSource(Generator generator) : generator(generator) {}

    virtual T next() override {
        return generator();
    }

    virtual T* next(size_t& count) override {
        T* newNext = new T[count];
        try {
            for (size_t i = 0; i < count; i++) {
                newNext[i] = generator();
            }
        }
        catch (...) {
            delete[] newNext;
            throw;
        }
        return newNext;
    }

    virtual bool hasNext() const override {
        return true;
    }

    virtual bool reset() override {
        return false;
    }
    virtual DataSource<T>* clone() const override
    {
        return new GeneratorDataSource(*this);
    }

    //The compiler will automatically generate a destructor

private:
    Generator generator;

};


char* generateRandomString() {
    static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz"; //less efficient not be static
    char* result = new char[11];

    for (int i = 0; i < 10; ++i) {
        result[i] = alphabet[rand() % 26];
    }
    result[10] = '\0';

    return result;
}


class PrimeGenerator
{
public:
    size_t operator()() {
        while (!isPrime(current)) {
            ++current;
        }
        return current++;
    }
private:
    bool isPrime(size_t number) {
        if (number <= 1) return false;
        if (number <= 3) return true;
        if (number % 2 == 0 || number % 3 == 0) return false;

        for (int i = 5; i * i <= number; i += 6) { 
            if (number % i == 0 || number % (i + 2) == 0) return false; // All primes greater than 3 can be written in the form 6k +- 1
        }                                                               // so it is more efficient than checking every number up to sqrt(number)
        return true;
    }
    size_t current{ 2 };
};


int generateRandomNumber() {
    return rand() % 100 + 1;  // chose  1-100
}

int* generateFibonacci() {
    int* fibonacci = new int[25];
    fibonacci[0] = 0;
    fibonacci[1] = 1;
    size_t current = 2;

    while (current >= 2 && current < 25) {
        fibonacci[current] = fibonacci[current - 1] + fibonacci[current - 2];
        current++;
    }

    return fibonacci; 
}


int main()
{
    /*Section for testing
    int arr[5] = { 1,2,3,4,5 };
    DataSource<int>* test = new ArrayDataSource<int>(arr, 5);
    
    while(test->hasNext())
        cout << test->operator()() << endl;*/

    DataSource<int>* primeSource = nullptr;
    DataSource<int>* randomSource = nullptr;
    DataSource<int>* fibonacciSource = nullptr;
    DataSource<int>* fileSource = nullptr;

    try {
        PrimeGenerator primeGenerator;
        primeSource = new GeneratorDataSource<int, PrimeGenerator>(primeGenerator);
        randomSource = new GeneratorDataSource<int, int(*)()>(generateRandomNumber);
        fibonacciSource = new ArrayDataSource<int>(generateFibonacci(), 25); 
        //if new fails it will be caught in the catch and will be delete after the try catch block (no problem deleting nullptr)


        GeneratorDataSource<char*, char* (*)()> stringSource(generateRandomString);

        for (int i = 0; i < 25; ++i) {
            char* str = stringSource.next();
            std::cout << str << std::endl;
            delete[] str;
        }

        DataSource<int>* sources[] = { primeSource, randomSource, fibonacciSource };
        AlternateDataSource<int> alternateSource(sources, 3);

        std::ofstream binaryFile("numbers.bin", std::ios::binary);
        if (!binaryFile.is_open()) {
            throw std::runtime_error("File could not be opened");
        }
        for (int i = 0; i < 1000; ++i) {
            int number = alternateSource.next();
            binaryFile.write(reinterpret_cast<char*>(&number), sizeof(number));
            if (!binaryFile) {
                throw std::runtime_error("Error writing to binary file");
            }
        }
        binaryFile.close();//Maybe no need for explicit close beacuse of RAII

        std::ifstream binaryIn("numbers.bin", std::ios::binary);
        if (!binaryIn.is_open()) {
            throw std::runtime_error("Could not open binary file");
        }

        std::ofstream textFile("numbers.txt");
        if (!textFile.is_open()) {
            throw std::runtime_error("Could not open text file");
        }

        int number{};
        while (binaryIn.read(reinterpret_cast<char*>(&number), sizeof(number))) {
            textFile << number << std::endl;

            if (!textFile) {
                if (textFile.fail()) {
                    throw std::runtime_error("Error writing to text file");
                }
                else if (textFile.bad()) {
                    throw std::runtime_error("A serious error occurred while writing to text file");
                }
            }
        }

        if (binaryIn.fail() && !binaryIn.eof()) {
            throw std::runtime_error("Error reading from binary file");
        }


        binaryIn.close();
        textFile.close();

        //Maybe no need for explicit close beacuse of RAII

        fileSource = new FileDataSource<int>("numbers.txt"); // maybe no need to be dyn
        while (fileSource->hasNext()) {
            std::cout << fileSource->next() << std::endl;
        }
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Runtime error caught: " << e.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
    }

    // Cleanup
    delete primeSource;
    delete randomSource;
    delete fibonacciSource;
    delete fileSource;

    return 0;
}
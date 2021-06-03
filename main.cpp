
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
using namespace std;

// Pixel structure
struct Pixel
{
    // Red, green, blue color values
    int red;
    int green;
    int blue;
};

/**
 * Gets an integer from a binary stream.
 * Helper function for read_image()
 * @param stream the stream
 * @param offset the offset at which to read the integer
 * @param bytes  the number of bytes to read
 * @return the integer starting at the given offset
 */ 
int get_int(fstream& stream, int offset, int bytes)
{
    stream.seekg(offset);
    int result = 0;
    int base = 1;
    for (int i = 0; i < bytes; i++)
    {   
        result = result + stream.get() * base;
        base = base * 256;
    }
    return result;
}

/**
 * Reads the BMP image specified and returns the resulting image as a vector
 * @param filename BMP image filename
 * @return the image as a vector of vector of Pixels
 */
vector<vector<Pixel> > read_image(string filename)
{
    // Open the binary file
    fstream stream;
    stream.open(filename, ios::in | ios::binary);

    // Get the image properties
    int file_size = get_int(stream, 2, 4);
    int start = get_int(stream, 10, 4);
    int width = get_int(stream, 18, 4);
    int height = get_int(stream, 22, 4);
    int bits_per_pixel = get_int(stream, 28, 2);

    // Scan lines must occupy multiples of four bytes
    int scanline_size = width * (bits_per_pixel / 8);
    int padding = 0;
    if (scanline_size % 4 != 0)
    {
        padding = 4 - scanline_size % 4;
    }

    // Return empty vector if this is not a valid image
    if (file_size != start + (scanline_size + padding) * height)
    {
        return {};
    }

    // Create a vector the size of the input image
    vector<vector<Pixel> > image(height, vector<Pixel> (width));

    int pos = start;
    // For each row, starting from the last row to the first
    // Note: BMP files store pixels from bottom to top
    for (int i = height - 1; i >= 0; i--)
    {
        // For each column
        for (int j = 0; j < width; j++)
        {
            // Go to the pixel position
            stream.seekg(pos);

            // Save the pixel values to the image vector
            // Note: BMP files store pixels in blue, green, red order
            image[i][j].blue = stream.get();
            image[i][j].green = stream.get();
            image[i][j].red = stream.get();

            // We are ignoring the alpha channel if there is one

            // Advance the position to the next pixel
            pos = pos + (bits_per_pixel / 8);
        }

        // Skip the padding at the end of each row
        stream.seekg(padding, ios::cur);
        pos = pos + padding;
    }

    // Close the stream and return the image vector
    stream.close();
    return image;
}

/**
 * Sets a value to the char array starting at the offset using the size
 * specified by the bytes.
 * This is a helper function for write_image()
 * @param arr    Array to set values for
 * @param offset Starting index offset
 * @param bytes  Number of bytes to set
 * @param value  Value to set
 * @return nothing
 */
void set_bytes(unsigned char arr[], int offset, int bytes, int value)
{
    for (int i = 0; i < bytes; i++)
    {
        arr[offset+i] = (unsigned char)(value>>(i*8));
    }
}

/**
 * Write the input image to a BMP file name specified
 * @param filename The BMP file name to save the image to
 * @param image    The input image to save
 * @return True if successful and false otherwise
 */
bool write_image(string filename, const vector<vector<Pixel> >& image)
{
    // Get the image width and height in pixels
    int width_pixels = image[0].size();
    int height_pixels = image.size();

    // Calculate the width in bytes incorporating padding (4 byte alignment)
    int width_bytes = width_pixels * 3;
    int padding_bytes = 0;
    padding_bytes = (4 - width_bytes % 4) % 4;
    width_bytes = width_bytes + padding_bytes;

    // Pixel array size in bytes, including padding
    int array_bytes = width_bytes * height_pixels;

    // Open a file stream for writing to a binary file
    fstream stream;
    stream.open(filename, ios::out | ios::binary);

    // If there was a problem opening the file, return false
    if (!stream.is_open())
    {
        return false;
    }

    // Create the BMP and DIB Headers
    const int BMP_HEADER_SIZE = 14;
    const int DIB_HEADER_SIZE = 40;
    unsigned char bmp_header[BMP_HEADER_SIZE] = {0};
    unsigned char dib_header[DIB_HEADER_SIZE] = {0};

    // BMP Header
    set_bytes(bmp_header,  0, 1, 'B');              // ID field
    set_bytes(bmp_header,  1, 1, 'M');              // ID field
    set_bytes(bmp_header,  2, 4, BMP_HEADER_SIZE+DIB_HEADER_SIZE+array_bytes); // Size of BMP file
    set_bytes(bmp_header,  6, 2, 0);                // Reserved
    set_bytes(bmp_header,  8, 2, 0);                // Reserved
    set_bytes(bmp_header, 10, 4, BMP_HEADER_SIZE+DIB_HEADER_SIZE); // Pixel array offset

    // DIB Header
    set_bytes(dib_header,  0, 4, DIB_HEADER_SIZE);  // DIB header size
    set_bytes(dib_header,  4, 4, width_pixels);     // Width of bitmap in pixels
    set_bytes(dib_header,  8, 4, height_pixels);    // Height of bitmap in pixels
    set_bytes(dib_header, 12, 2, 1);                // Number of color planes
    set_bytes(dib_header, 14, 2, 24);               // Number of bits per pixel
    set_bytes(dib_header, 16, 4, 0);                // Compression method (0=BI_RGB)
    set_bytes(dib_header, 20, 4, array_bytes);      // Size of raw bitmap data (including padding)                     
    set_bytes(dib_header, 24, 4, 2835);             // Print resolution of image (2835 pixels/meter)
    set_bytes(dib_header, 28, 4, 2835);             // Print resolution of image (2835 pixels/meter)
    set_bytes(dib_header, 32, 4, 0);                // Number of colors in palette
    set_bytes(dib_header, 36, 4, 0);                // Number of important colors

    // Write the BMP and DIB Headers to the file
    stream.write((char*)bmp_header, sizeof(bmp_header));
    stream.write((char*)dib_header, sizeof(dib_header));

    // Initialize pixel and padding
    unsigned char pixel[3] = {0};
    unsigned char padding[3] = {0};

    // Pixel Array (Left to right, bottom to top, with padding)
    for (int h = height_pixels - 1; h >= 0; h--)
    {
        for (int w = 0; w < width_pixels; w++)
        {
            // Write the pixel (Blue, Green, Red)
            pixel[0] = image[h][w].blue;
            pixel[1] = image[h][w].green;
            pixel[2] = image[h][w].red;
            stream.write((char*)pixel, 3);
        }
        // Write the padding bytes
        stream.write((char *)padding, padding_bytes);
    }

    // Close the stream and return true
    stream.close();
    return true;
}


vector<vector<Pixel> > process_1(const vector<vector<Pixel> >& image)
// Adds vignette effect to image (dark corners)
// read in an image, process the pixel values using Process 1, and write the result out to a new image file.
{
    // Get the number of rows/columns from the input 2D vector
    int width_pixels = image[0].size();
    int height_pixels = image.size();


    // Define a new 2D vector the same size as the input 2D vector
    vector<vector<Pixel> > new_image(height_pixels, vector<Pixel> (width_pixels));

    // Iterate through the pixels of the input 2D vector (nested loop)
    for (int row = 0; row < height_pixels; row++) { // height (a.k.a. number of rows) 
        for (int col = 0; col < width_pixels; col++) { // width (a.k.a. number of columns)
            
            // Get the color values for a single pixel in the input 2D vector
            int red = image[row][col].red;
            int green = image[row][col].green;
            int blue = image[row][col].blue;

            
            double distance = sqrt(pow((col - width_pixels/2), 2) + pow((row - height_pixels/2), 2));
            double scaling_factor = (height_pixels - distance)/height_pixels;


            // Save the new color values to the corresponding pixel in the new 2D vector
            new_image[row][col].red = red * scaling_factor;
            new_image[row][col].green = green * scaling_factor;
            new_image[row][col].blue = blue * scaling_factor;
            

           /* Block to test that copying image correctly
           new_image[row][col].red = red;
           new_image[row][col].green = green;
           new_image[row][col].blue = blue;
           */
            
        }
    }
        
    // Return the new 2D vector
    return new_image;
}

vector<vector<Pixel> > process_2(const vector<vector<Pixel> >& image, double scaling_factor) {
    // Adds Clarendon effect to image (darks darker and lights lighter) by a scaling factor

    // Get the number of rows/columns from the input 2D vector
    int width_pixels = image[0].size();
    int height_pixels = image.size();

    // Define a new 2D vector the same size as the input 2D vector
    vector<vector<Pixel> > new_image(height_pixels, vector<Pixel> (width_pixels));

    // Iterate through the pixels of the input 2D vector (nested loop)
    for (int row = 0; row < height_pixels; row++) { // height (a.k.a. number of rows) 
        for (int col = 0; col < width_pixels; col++) { // width (a.k.a. number of columns)

        // Get the color values for a single pixel in the input 2D vector
        int red = image[row][col].red;
        int green = image[row][col].green;
        int blue = image[row][col].blue;

        // Perform the operation on the color values (refer to Runestone for this)
        double average_value = (red + green + blue)/3;
        int new_red, new_green, new_blue;

        
        if (average_value >= 170) {
            new_red = 255 - (255 - red) * scaling_factor;
            new_green = 255 - (255 - green) * scaling_factor;
            new_blue = 255 - (255 - blue) * scaling_factor;
        } else if (average_value < 90) {
            new_red = red * scaling_factor;
            new_green = green * scaling_factor;
            new_blue = blue * scaling_factor;
        } else {
            new_red = red;
            new_green = green;
            new_blue = blue;
        }

        // Save the new color values to the corresponding pixel in the new 2D vector
        new_image[row][col].red = new_red;
        new_image[row][col].green = new_green;
        new_image[row][col].blue = new_blue;
        }
    }

    // Return the new 2D vector
    return new_image;

}

vector<vector<Pixel> > process_3(const vector<vector<Pixel> >& image) {
    // Grayscale image
    
    // Get the number of rows/columns from the input 2D vector
    int width_pixels = image[0].size();
    int height_pixels = image.size();

    // Define a new 2D vector the same size as the input 2D vector
    vector<vector<Pixel> > new_image(height_pixels, vector<Pixel> (width_pixels));

    // Iterate through the pixels of the input 2D vector (nested loop)
    for (int row = 0; row < height_pixels; row++) { // height (a.k.a. number of rows) 
        for (int col = 0; col < width_pixels; col++) { // width (a.k.a. number of columns)

        // Get the color values for a single pixel in the input 2D vector
        int red = image[row][col].red;
        int green = image[row][col].green;
        int blue = image[row][col].blue;

        // To round a positive floating-point value to the nearest integer, 
        // add 0.5 and then convert to an integer. (For a negative value, you subtract 0.5.)
        int gray_value = ((red + green + blue)/3) + 0.5;

        // Save the new color values to the corresponding pixel in the new 2D vector
        new_image[row][col].red = gray_value; 
        new_image[row][col].green = gray_value;
        new_image[row][col].blue = gray_value;
        }
    }

    // Return the new 2D vector
    return new_image;

}

vector<vector<Pixel> > process_4(const vector<vector<Pixel> >& image){
    // Rotates image by 90 degrees clockwise (not counter-clockwise)
    
    // Get the number of rows/columns from the input 2D vector
    int width_pixels = image[0].size();
    int height_pixels = image.size();

    // Define a new 2D vector the same size as the input 2D vector
    vector<vector<Pixel> > new_image(width_pixels, vector<Pixel> (height_pixels)); // height and width switched

    // Iterate through the pixels of the input 2D vector (nested loop)
    for (int row = 0; row < height_pixels; row++) { // height (a.k.a. number of rows) 
        for (int col = 0; col < width_pixels; col++) { // width (a.k.a. number of columns)

        // Get the color values for a single pixel in the input 2D vector
        int red = image[row][col].red;
        int green = image[row][col].green;
        int blue = image[row][col].blue;

        // Perform the operation on the color values (refer to Runestone for this)

        // Save the new color values to the corresponding pixel in the new 2D vector
        new_image[col][(height_pixels - 1) - row].red = red; 
        new_image[col][(height_pixels - 1) - row].green = green;
        new_image[col][(height_pixels - 1) - row].blue = blue;
        }
    }
    // Return the new 2D vector
    return new_image;

}

vector<vector<Pixel> > process_5(const vector<vector<Pixel> >& image, int number) { // TODO
    // Rotates image by a specified number of multiples of 90 degrees clockwise

    int angle = number * 90;
    if (angle % 90 != 0) {
        cout << "angle must be a multiple of 90 degrees. Please enter a valid number of rotations." << endl;
        return image;
    } else if (angle % 360 == 0) {
        return image;
    } else if (angle % 360 == 90) {
        return process_4(image);
    } else if (angle % 360 == 180) {
        return process_4(process_4(image));
    } else {
        return process_4(process_4(process_4(image)));
    }
}

vector<vector<Pixel> > process_6(const vector<vector<Pixel> >& image, int x_scale, int y_scale){
    // Enlarges the image in the x and y direction
    
    // Get the number of rows/columns from the input 2D vector
    int width_pixels = image[0].size();
    int height_pixels = image.size();

    // Define a new 2D vector the same size as the input 2D vector
    vector<vector<Pixel> > new_image(height_pixels*y_scale, vector<Pixel> (width_pixels*x_scale));

    // Iterate through the pixels of the input 2D vector (nested loop)
    for (int row = 0; row < height_pixels*y_scale; row++) { // height (a.k.a. number of rows) 
        for (int col = 0; col < width_pixels*x_scale; col++) { // width (a.k.a. number of columns)

        // Get the color values for a single pixel in the input 2D vector
        int red = image[(row/y_scale)+0.5][(col/x_scale)+0.5].red;
        int green = image[(row/y_scale)+0.5][(col/x_scale)+0.5].green;
        int blue = image[(row/y_scale)+0.5][(col/x_scale)+0.5].blue;

        // Perform the operation on the color values (refer to Runestone for this)
        // python Image -> image.setPixel(x,y,rgb)

        // Save the new color values to the corresponding pixel in the new 2D vector
        new_image[row][col].red = red; 
        new_image[row][col].green = green;
        new_image[row][col].blue = blue;
        }
    }

    // Return the new 2D vector
    return new_image;
}

vector<vector<Pixel> > process_7(const vector<vector<Pixel> >& image) {
    // Convert image to high contrast (black and white only)
    
    // Get the number of rows/columns from the input 2D vector
    int width_pixels = image[0].size();
    int height_pixels = image.size();


    // Define a new 2D vector the same size as the input 2D vector
    vector<vector<Pixel> > new_image(height_pixels, vector<Pixel> (width_pixels));

    // Iterate through the pixels of the input 2D vector (nested loop)
    for (int row = 0; row < height_pixels; row++) { // height (a.k.a. number of rows) 
        for (int col = 0; col < width_pixels; col++) { // width (a.k.a. number of columns)
            
            // Get the color values for a single pixel in the input 2D vector
            int red = image[row][col].red;
            int green = image[row][col].green;
            int blue = image[row][col].blue;

            // Perform the operation on the color values (refer to Runestone for this)
            int gray_value = (red + green + blue)/3;
            int new_red, new_green, new_blue;

            if (gray_value >= 255/2) {
                new_red = 255;
                new_green = 255;
                new_blue = 255;
            } else {
                new_red = 0;
                new_green = 0;
                new_blue = 0;
            }

            // Save the new color values to the corresponding pixel in the new 2D vector
            new_image[row][col].red = new_red;
            new_image[row][col].green = new_green;
            new_image[row][col].blue = new_blue;
        }
    }
        
    // Return the new 2D vector
    return new_image;
}

vector<vector<Pixel> > process_8(const vector<vector<Pixel> >& image, double scaling_factor) {
    // Lightens image by a scaling factor
    
    // Get the number of rows/columns from the input 2D vector
    int width_pixels = image[0].size();
    int height_pixels = image.size();

    // Define a new 2D vector the same size as the input 2D vector
    vector<vector<Pixel> > new_image(height_pixels, vector<Pixel> (width_pixels));

    // Iterate through the pixels of the input 2D vector (nested loop)
    for (int row = 0; row < height_pixels; row++) { // height (a.k.a. number of rows) 
        for (int col = 0; col < width_pixels; col++) { // width (a.k.a. number of columns)
            
            // Get the color values for a single pixel in the input 2D vector
            int red = image[row][col].red;
            int green = image[row][col].green;
            int blue = image[row][col].blue;

            // Perform the operation on the color values (refer to Runestone for this)
            //DO THE SPECIAL STUFF HERE
            int new_red, new_green, new_blue;

            new_red = 255 - (255 - red) * scaling_factor;
            new_green = 255 - (255 - green) * scaling_factor;
            new_blue = 255 - (255 - blue) * scaling_factor;

            // Save the new color values to the corresponding pixel in the new 2D vector
            new_image[row][col].red = new_red;
            new_image[row][col].green = new_green;
            new_image[row][col].blue = new_blue;
        }
    }
        
    // Return the new 2D vector
    return new_image;
}

vector<vector<Pixel> > process_9(const vector<vector<Pixel> >& image, double scaling_factor) {
    // Darkens image by a scaling factor
    // Get the number of rows/columns from the input 2D vector
    int width_pixels = image[0].size();
    int height_pixels = image.size();

    // Define a new 2D vector the same size as the input 2D vector
    vector<vector<Pixel> > new_image(height_pixels, vector<Pixel> (width_pixels));

    // Iterate through the pixels of the input 2D vector (nested loop)
    for (int row = 0; row < height_pixels; row++) { // height (a.k.a. number of rows) 
        for (int col = 0; col < width_pixels; col++) { // width (a.k.a. number of columns)
            
            // Get the color values for a single pixel in the input 2D vector
            int red = image[row][col].red;
            int green = image[row][col].green;
            int blue = image[row][col].blue;

            int new_red, new_green, new_blue;

            new_red = red * scaling_factor;
            new_green = green * scaling_factor;
            new_blue = blue * scaling_factor;

            // Save the new color values to the corresponding pixel in the new 2D vector
            new_image[row][col].red = new_red;
            new_image[row][col].green = new_green;
            new_image[row][col].blue = new_blue;
        }
    }
        
    // Return the new 2D vector
    return new_image;
}

vector<vector<Pixel> > process_10(const vector<vector<Pixel> >& image) {
    // Converts image to only black, white, red, blue, and green
    // Get the number of rows/columns from the input 2D vector
    int width_pixels = image[0].size();
    int height_pixels = image.size();


    // Define a new 2D vector the same size as the input 2D vector
    vector<vector<Pixel> > new_image(height_pixels, vector<Pixel> (width_pixels));

    // Iterate through the pixels of the input 2D vector (nested loop)
    for (int row = 0; row < height_pixels; row++) { // height (a.k.a. number of rows) 
        for (int col = 0; col < width_pixels; col++) { // width (a.k.a. number of columns)
            
            // Get the color values for a single pixel in the input 2D vector
            int red = image[row][col].red;
            int green = image[row][col].green;
            int blue = image[row][col].blue;

            // Perform the operation on the color values
            int max_color = max(max(red, green), blue);
            int new_red, new_green, new_blue;

            if (red + green + blue >= 550) {
                new_red = 255;
                new_green = 255;
                new_blue = 255;
            } else if (red + green + blue <= 150) {
                new_red = 0;
                new_green = 0;
                new_blue = 0;
            } else if (max_color == red) {
                new_red = 255;
                new_green = 0;
                new_blue = 0;
            } else if (max_color == green) {
                new_red = 0;
                new_green = 255;
                new_blue = 0;
            } else {
                new_red = 0;
                new_green = 0;
                new_blue = 255;
            }

            // Save the new color values to the corresponding pixel in the new 2D vector
            new_image[row][col].red = new_red;
            new_image[row][col].green = new_green;
            new_image[row][col].blue = new_blue;
        }
    }
        
    // Return the new 2D vector
    return new_image;
}

int main()
{

    cout <<"Image Processing Application" << endl << endl;

    bool isDone = false;
    string menuSelect;
    string filename, outputFilename;

    cout << "Enter input BMP filename: ";
    cin >> filename;

    // get image from file name
    // Read in BMP image file into a 2D vector (using read_image function)
    vector<vector<Pixel> > image = read_image(filename);

    while (!isDone) {

        // TODO: handle incorrect user input -> restart application
        if (cin.fail()) {
            cout << "Please restart application, and enter a valid filename";
            break;
        }

        cout << "\n\nIMAGE PROCESSING MENU" << endl;
        cout << "0) Change image (current: " << filename << ")" << endl;
        cout << "1) Vignette" << endl;
        cout << "2) Clarendon" << endl;
        cout << "3) Grayscale" << endl;
        cout << "4) Rotate 90 degrees" << endl;
        cout << "5) Rotate multiple 90 degrees" << endl;
        cout << "6) Enlarge" << endl;
        cout << "7) High contrast" << endl;
        cout << "8) Lighten" << endl;
        cout << "9) Darken" << endl;
        cout << "10) Black, white, red, green, blue" << endl;

        cout << "\n\nEnter menu selection (Q to quit): ";
        cin >> menuSelect;

        if (menuSelect == "Q" || menuSelect == "q"){
            cout << "Thank you for using my program!" << endl;
            cout << "Quitting.." << endl;
            isDone = true;
        } else {
            int numChoice = stoi(menuSelect); // change selection to int for menu selction if not Q
            switch (numChoice){
                case 0: {
                    cout << "Change image selected" << endl;
                    cout << "Enter input BMP filename: ";
                    cin >> filename;
                    cout << "Successfully changed input image!" << endl;
                    break;
                }
                case 1: {
                    cout << "Vignette selected" << endl;
                    cout << "Enter output BMP filename: ";
                    cin >> outputFilename;

                    // Call process_1 function using the 2D vector and save the resulting 2D vector that is returned
                    vector<vector<Pixel> > new_image = process_1(image);

                    // Write the resulting 2D vector to a new BMP image file (using write_image function)
                    if (write_image(outputFilename, new_image)) {
                        cout << "Successfully applied vignette!" << endl;
                    }
                    break;
                }
                    //
                case 2: {
                    cout << "Clarendon selected" << endl;
                    cout << "Enter output BMP filename: ";
                    cin >> outputFilename;
                    cout << "Enter scaling factor: ";
                    double scalingFactor;
                    cin >> scalingFactor;

                    // Call process_2 function using the 2D vector and save the resulting 2D vector that is returned
                    vector<vector<Pixel> > new_image = process_2(image, scalingFactor);

                    // Write the resulting 2D vector to a new BMP image file (using write_image function)
                    if (write_image(outputFilename, new_image)) {
                        cout << "Successfully applied clarendon!" << endl;
                    }
                    
                    break;
                }
                case 3: {

                    cout << "Grayscale selected" << endl;
                    cout << "Enter output BMP filename: ";
                    cin >> outputFilename;

                    // Call process_3 function using the 2D vector and save the resulting 2D vector that is returned
                    vector<vector<Pixel> > new_image = process_3(image);

                    // Write the resulting 2D vector to a new BMP image file (using write_image function)
                    if (write_image(outputFilename, new_image)) {
                        cout << "Successfully applied grayscale!" << endl;
                    }

                    break;
                }
                case 4: {
                    cout << "Rotate 90 degrees selected" << endl;
                    cout << "Enter output BMP filename: ";
                    cin >> outputFilename;

                    // Call process_4 function using the 2D vector and save the resulting 2D vector that is returned
                    vector<vector<Pixel> > new_image = process_4(image);

                    // Write the resulting 2D vector to a new BMP image file (using write_image function)
                    if (write_image(outputFilename, new_image)) {
                        cout << "Successfully applied 90 degree rotation!" << endl;
                    }

                    break;
                }
                case 5: {

                    cout << "Rotate multiple 90 degrees selected" << endl;
                    cout << "Enter output BMP filename: ";
                    cin >> outputFilename;
                    int rotationNum;
                    cout << "\nEnter number of 90 degree rotations: ";
                    cin >> rotationNum;

                    // Call process_5 function using the 2D vector and save the resulting 2D vector that is returned
                    vector<vector<Pixel> > new_image = process_5(image, rotationNum);

                    // Write the resulting 2D vector to a new BMP image file (using write_image function)
                    if (write_image(outputFilename, new_image)) {
                        cout << "Successfully applied multiple 90 degree rotations!" << endl;
                    }

                    break;
                }
                case 6: {
                    cout << "Enlarge selected" << endl;
                    cout << "Enter output BMP filename: ";
                    cin >> outputFilename;
                    
                    int x_scale, y_scale;
                    cout << "Enter X scale: ";
                    cin >> x_scale;
                    cout << "\nEnter Y scale: ";
                    cin >> y_scale;

                    // Call process_6 function using the 2D vector and save the resulting 2D vector that is returned
                    vector<vector<Pixel> > new_image = process_6(image, x_scale, y_scale);

                    // Write the resulting 2D vector to a new BMP image file (using write_image function)
                    if (write_image(outputFilename, new_image)) {
                        cout << "Successfully enlarged!" << endl;
                    }

                    break;
                }
                case 7: {
                    cout << "High contrast selected" << endl;
                    cout << "Enter output BMP filename: ";
                    cin >> outputFilename;

                    // Call process_7 function using the 2D vector and save the resulting 2D vector that is returned
                    vector<vector<Pixel> > new_image = process_7(image);

                    // Write the resulting 2D vector to a new BMP image file (using write_image function)
                    if (write_image(outputFilename, new_image)) {
                        cout << "Successfully applied high contrast!" << endl;
                    }
                    
                    break;
                }
                case 8: {

                    cout << "Lighten selected" << endl;
                    cout << "Enter output BMP filename: ";
                    cin >> outputFilename;
                    cout << "Enter scaling factor: ";
                    double scalingFactor;
                    cin >> scalingFactor;

                    // Call process_8 function using the 2D vector and save the resulting 2D vector that is returned
                    vector<vector<Pixel> > new_image = process_8(image, scalingFactor);

                    // Write the resulting 2D vector to a new BMP image file (using write_image function)
                    if (write_image(outputFilename, new_image)) {
                        cout << "Successfully lightened!" << endl;
                    }


                    break;
                }
                case 9: {

                    cout << "Darken selected" << endl;
                    cout << "Enter output BMP filename: ";
                    cin >> outputFilename;
                    cout << "Enter scaling factor: ";
                    double scalingFactor;
                    cin >> scalingFactor;

                    // Call process_9 function using the 2D vector and save the resulting 2D vector that is returned
                    vector<vector<Pixel> > new_image = process_9(image, scalingFactor);

                    // Write the resulting 2D vector to a new BMP image file (using write_image function)
                    if (write_image(outputFilename, new_image)) {
                        cout << "Successfully darkened!" << endl;
                    }
                    break;
                }
                case 10: {
                    cout << "Black, white, red, green, blue selected" << endl;
                    cout << "Enter output BMP filename: ";
                    cin >> outputFilename;

                    // Call process_1 function using the 2D vector and save the resulting 2D vector that is returned
                    vector<vector<Pixel> > new_image = process_10(image);

                    // Write the resulting 2D vector to a new BMP image file (using write_image function)
                    if (write_image(outputFilename, new_image)) {
                        cout << "Successfully applied black, white, red, green, blue filter!" << endl;
                    }

                    break;
                }
                default: {
                    cout << "Invalid menu selection. Please restart application, and try again." << endl;
                    isDone = true;
                }
            }
            // TODO: maybe add a sleep timer here so the next menu doesn't appear so quickly..?
        }


        

    }

    return 0;
}
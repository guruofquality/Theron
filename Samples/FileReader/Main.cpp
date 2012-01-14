// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This sample demonstrates the use of an actor to accomplish an asynchronous
// task execution that might more typically be achieved using threads and
// explicit thread synchronization. In this example we define a FileReader
// actor that provides a file reading service to clients. Clients request a file
// to be read using a ReadFileMessage, providing the path of the file and a pointer
// to a buffer to be filled with the contents. The reading of the file is performed
// asynchronously and the FileReader returns a FileMessage to the client when reading
// is complete. The FileMessage contains the actual length of the file read, with
// zero indicating an error of some kind. In this simple example the main thread,
// which acts as the client, simply waits for the FileMessage to be returned, going
// to sleep. More typically it would continue to perform other tasks in parallel,
// such as responding to input or updating the display. The Receiver class, used to
// receive the returned FileMessage message, exposes a Count() method, as well as the
// Wait() message called here. The Count() method can be called to check for received
// messages without blocking until one arrives.
//


#include <stdio.h>

#include <Theron/Actor.h>
#include <Theron/Address.h>
#include <Theron/Framework.h>
#include <Theron/Receiver.h>


// A file read request message.
struct ReadFileMessage
{
    const char *mFilename;          // String containing the path to the file to be read.
    unsigned char *mBuffer;         // Pointer to a data buffer to be filled.
    unsigned int mBufferSize;       // Size of the buffer, ie. maximum file data size.
};


// A file message, indicating that a file has been read.
struct FileMessage
{
    size_t mFileSize;				// Actual size of the file data in bytes.
};


// Actor that reads disk files asynchronously for a client.
class FileReader : public Theron::Actor
{
public:

    // Constructor.
    inline FileReader()
    {
        RegisterHandler(this, &FileReader::Handler);
    }

private:

    // Handler for ReadFileMessage messages.
    inline void Handler(const ReadFileMessage &message, const Theron::Address from)
    {
        // Prepare a File message to be returned to the sender.
        // Default file size to zero indicating failure.
        FileMessage fileMessage;
        fileMessage.mFileSize = 0;

        // Try to open the file
        FILE *handle = fopen(message.mFilename, "rb");
        if (handle != 0)
        {
            // Read the file data, setting the actual size.
            fileMessage.mFileSize = fread(
                message.mBuffer,
                sizeof(unsigned char),
                message.mBufferSize,
				handle);

            fclose(handle);
        }

        // Send the File message back to the sender.
        Send(fileMessage, from);
    }
};


// A helper that handles File messages received by a Receiver.
class MessageCollector
{
public:

    void Handler(const FileMessage &message, const Theron::Address /*from*/)
    {
        mFileMessage = message;
    }

    FileMessage mFileMessage;
};


int main(int argc, char *argv[])
{
    const char *filename = 0;
    if (argc > 1)
    {
        filename = argv[1];
    }

    if (filename == 0)
    {
        printf("No filename supplied. Use command line argument to supply one.\n");
        return 1;
    }

    printf("Reading file from path '%s'.\n", filename);

    Theron::Framework framework;
    Theron::ActorRef fileReader(framework.CreateActor<FileReader>());

    MessageCollector messageCollector;
    Theron::Receiver receiver;
    receiver.RegisterHandler(&messageCollector, &MessageCollector::Handler);

    // Allocate a buffer for the file data.
    const unsigned int MAX_FILE_SIZE = 65536;
    unsigned char fileBuffer[MAX_FILE_SIZE];

    // Send the actor a message to request the file read operation.
    ReadFileMessage readFileMessage;
    readFileMessage.mFilename = filename;
    readFileMessage.mBuffer = fileBuffer;
    readFileMessage.mBufferSize = MAX_FILE_SIZE;

    fileReader.Push(readFileMessage, receiver.GetAddress());

    // Wait for a reply message indicating the file has been read.
    // This is a blocking call and so prevents this thread from doing
    // any more work in parallel. In practice it would be better to
    // call receiver.Count() periodically while doing other work (but
    // not so often that we busy-wait!). The Count() method is
    // non-blocking and just returns the number of messages received
    // but not yet waited for.
    receiver.Wait();

    // Check the returned file size.
    printf("Read %d bytes\n", messageCollector.mFileMessage.mFileSize);

    return 0;
}


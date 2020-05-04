#include "GifWriterJob.h"
#include "gif-h/gif.h"

GifWriterJob::GifWriterJob(const char* filename, uint32_t delay, std::vector<uint8_t>& framesData): filename(filename),
                                                                                                    delay(delay),
                                                                                                    framesData(
                                                                                                       framesData)
{
}

void GifWriterJob::Notify()
{
   std::unique_lock<std::mutex> lock(mutexCV);
   ready = true;
   conditionVariable.notify_one();
}

void GifWriterJob::Run()
{
   GifWriter gifWriter;

   GifBegin(&gifWriter, filename, 64, 64, delay);
   for (int i = 0; i < 600; ++i)
      GifWriteFrame(&gifWriter, &(framesData.data()[i * 64 * 64 * 4]), 64, 64, delay);

   GifEnd(&gifWriter);
}

void GifWriterJob::RunAsync()
{
   job = std::thread([&]()
   {
      std::unique_lock<std::mutex> lock(mutexCV);
      while (!ready) conditionVariable.wait(lock);
      Run();
   });
}

void GifWriterJob::Wait()
{
   job.join();
}

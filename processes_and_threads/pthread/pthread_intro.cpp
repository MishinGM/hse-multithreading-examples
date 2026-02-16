#include <utils/error.hpp>

#include <pthread.h>

#include <format>
#include <iostream>

struct Context {
  int a{};
  int b{};
};

void *ThreadFunction(void *ctx) {
  auto *context = static_cast<Context *>(ctx);

  std::cout << std::format(
                   "[CHILD]: context->a address: {}, context->b address: {}",
                   static_cast<const void *>(&(context->a)),
                   static_cast<const void *>(&(context->b)))
            << std::endl;
    
  context->a += 10;

  std::cout << std::format(
                   "[CHILD]: context->a value: {}, context->b value: {}",
                   context->a, context->b)
            << std::endl;

  return nullptr;
}

int main() {
  Context ctx{.a = 228, .b = 322};
  pthread_t thread{};

  std::cout << std::format(
                   "[PARENT]: context->a address: {}, context->b address: {}",
                   static_cast<const void *>(&(ctx.a)),
                   static_cast<const void *>(&(ctx.b)))
            << std::endl;

  CHECK_ERROR(pthread_create(&thread, nullptr, ThreadFunction, &ctx));
  CHECK_ERROR(pthread_join(thread, nullptr));

  std::cout << std::format(
                   "[PARENT]: context->a value: {}, context->b value: {}", ctx.a,
                   ctx.b)
            << std::endl;

  return EXIT_SUCCESS;
}
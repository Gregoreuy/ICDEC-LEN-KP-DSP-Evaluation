################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_add_s16.c \
../Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_add_s8.c \
../Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_mul_s16.c \
../Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_mul_s8.c 

C_DEPS += \
./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_add_s16.d \
./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_add_s8.d \
./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_mul_s16.d \
./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_mul_s8.d 

OBJS += \
./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_add_s16.o \
./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_add_s8.o \
./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_mul_s16.o \
./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_mul_s8.o 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/%.o Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/%.su Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/%.cyclo: ../Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/%.c Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U5A9xx -c -I../Core/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/ -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-Third_Party-2f-EdgeImpulse_Yes-2d-No_MachineLearning-2f-edgeimpulse-2f-edge-2d-impulse-2d-sdk-2f-CMSIS-2f-NN-2f-Source-2f-BasicMathFunctions

clean-Middlewares-2f-Third_Party-2f-EdgeImpulse_Yes-2d-No_MachineLearning-2f-edgeimpulse-2f-edge-2d-impulse-2d-sdk-2f-CMSIS-2f-NN-2f-Source-2f-BasicMathFunctions:
	-$(RM) ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_add_s16.cyclo ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_add_s16.d ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_add_s16.o ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_add_s16.su ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_add_s8.cyclo ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_add_s8.d ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_add_s8.o ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_add_s8.su ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_mul_s16.cyclo ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_mul_s16.d ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_mul_s16.o ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_mul_s16.su ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_mul_s8.cyclo ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_mul_s8.d ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_mul_s8.o ./Middlewares/Third_Party/EdgeImpulse_Yes-No_MachineLearning/edgeimpulse/edge-impulse-sdk/CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_mul_s8.su

.PHONY: clean-Middlewares-2f-Third_Party-2f-EdgeImpulse_Yes-2d-No_MachineLearning-2f-edgeimpulse-2f-edge-2d-impulse-2d-sdk-2f-CMSIS-2f-NN-2f-Source-2f-BasicMathFunctions


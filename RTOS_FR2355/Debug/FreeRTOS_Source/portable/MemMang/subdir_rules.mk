################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
FreeRTOS_Source/portable/MemMang/%.obj: ../FreeRTOS_Source/portable/MemMang/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccs1200/ccs/tools/compiler/ti-cgt-msp430_16.9.11.LTS/bin/cl430" -vmspx --data_model=large --use_hw_mpy=F5 --include_path="C:/ti/ccs1200/ccs/ccs_base/msp430/include" --include_path="C:/Users/gzm20/Desktop/magic-wifi/RTOS_FR2355/FreeRTOS_Source/include" --include_path="C:/Users/gzm20/Desktop/magic-wifi/RTOS_FR2355/FreeRTOS_Source/portable/CCS/MSP430X" --include_path="C:/Users/gzm20/Desktop/magic-wifi/RTOS_FR2355/driverlib/MSP430FR2xx_4xx" --include_path="C:/Users/gzm20/Desktop/magic-wifi/RTOS_FR2355" --include_path="C:/ti/ccs1200/ccs/tools/compiler/ti-cgt-msp430_16.9.11.LTS/include" --advice:power=all --advice:hw_config="all" --define=__MSP430FR2355__ --define=_FRWP_ENABLE --define=_INFO_FRWP_ENABLE -g --printf_support=full --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="FreeRTOS_Source/portable/MemMang/$(basename $(<F)).d_raw" --obj_directory="FreeRTOS_Source/portable/MemMang" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '



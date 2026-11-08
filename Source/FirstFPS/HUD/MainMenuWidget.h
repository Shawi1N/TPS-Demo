#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UEditableTextBox;

UCLASS()
class FIRSTFPS_API UMainMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 显示菜单后，将初始焦点放到创建房间按钮。
    void FocusDefaultButton();

protected:
    /* 用于每个运行时 Widget 实例的一次性初始化，适合绑定按钮 */
    virtual void NativeOnInitialized() override;

private:
    UPROPERTY(meta = (BindWidget))
    UButton* HostButton = nullptr;

    UPROPERTY(meta = (BindWidget))
    UButton* JoinButton = nullptr;

    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* IPAddressInput = nullptr;
    
    UPROPERTY(meta = (BindWidget))
    UButton* QuitButton = nullptr;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* StatusText = nullptr;

    UFUNCTION()
    void HandleHostClicked();

    UFUNCTION()
    void HandleJoinClicked();

    UFUNCTION()
    void HandleQuitClicked();

    void SetStatus(const FText& Message);
};
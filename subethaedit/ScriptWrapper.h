//
//  ScriptWrapper.h
//  SubEthaEdit
//
//  Created by Dominik Wagner on 04.04.06.
//  Copyright 2006 TheCodingMonkeys. All rights reserved.
//

#import <Cocoa/Cocoa.h>

extern NSString * const ScriptWrapperDisplayNameSettingsKey;
extern NSString * const ScriptWrapperKeyboardShortcutSettingsKey;
extern NSString * const ScriptWrapperToolbarIconSettingsKey;
extern NSString * const ScriptWrapperInDefaultToolbarSettingsKey;

@interface ScriptWrapper : NSObject {
    NSAppleScript *I_appleScript;
    NSURL         *I_URL;
    NSDictionary  *I_settingsDictionary;
    NSMutableSet *I_tasks;
}

+ (id)scriptWrapperWithContentsOfURL:(NSURL *)anURL;

- (id)initWithContentsOfURL:(NSURL *)anURL;

- (void)executeAndReturnError:(NSDictionary **)errorDictionary;
- (NSDictionary *)settingsDictionary;
- (void)revealSource;

@end
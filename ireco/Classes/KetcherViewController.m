//
//  KetcherViewController.m
//  ireco
//
//  Created by Boris Karulin on 07.12.10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "KetcherViewController.h"
#import "Recognizer.h"

@implementation KetcherViewController

@synthesize webView, activityView, molfile, prevImage;

#pragma mark -
#pragma mark KetcherViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
   self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
   return self;
}

- (void)viewDidLoad
{
   NSError *error = nil;
   NSString *html = [[NSString alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"ketcher" ofType:@"html"] encoding:NSUTF8StringEncoding error:&error];
   [webView loadHTMLString:html baseURL:[NSURL fileURLWithPath: [[NSBundle mainBundle] bundlePath]]];
   [html release];
}

- (void)viewDidUnload
{
}

- (void)dealloc
{	
   
   [webView release];
   [activityView release];
   
   if (self.molfile != nil)
      [self.molfile release];

   [super dealloc];
}

- (void)setupKetcher:(UIImage *)image
{
   [self.activityView startAnimating];
   
   NSThread* recognizerThread = [[NSThread alloc] initWithTarget:self
                                                        selector:@selector(recognizingProc:)
                                                          object:image];
   [recognizerThread start];
   [recognizerThread autorelease];
}

- (void)recognizingProc:(UIImage *)image
{
   if (prevImage == nil || prevImage != image)
   {
      Recognizer *recognizer = [Recognizer recognizerWithImage:image];
      
      if (self.molfile != nil)
         [self.molfile release];
      self.molfile = [[recognizer recognize] retain];
   }

   self.prevImage = image;
   
   if (self.molfile != nil)
   {
      NSString *jsLoadMol = [NSString stringWithFormat: @"ketcher.setMolecule('%@');", [self.molfile stringByReplacingOccurrencesOfString:@"\n" withString:@"\\n"]];
      [self.webView performSelectorOnMainThread:@selector(stringByEvaluatingJavaScriptFromString:) withObject: jsLoadMol waitUntilDone: YES];
   } else {
      [webView loadHTMLString:@"<html><head></head><body>An error occured.</body></html>" baseURL:nil];
   }
   
   [self.activityView stopAnimating];
}

- (void)webViewDidFinishLoad:(UIWebView *)webView
{
}

// called when the parent application receives a memory warning
- (void)didReceiveMemoryWarning
{
   // we have been warned that memory is getting low, stop all timers
   //
   [super didReceiveMemoryWarning];
}

@end